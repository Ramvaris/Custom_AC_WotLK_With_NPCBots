/*
 * Solo Sustain Module
 * Author: Ramvaris
 * 
 * Passive life and mana leech based on player damage dealt.
 * Player heals themselves AND their active pets/guardians from their own damage.
 * 
 * Design: Player deals damage → Player heals → Pet/Guardian also heals
 * Pets/Guardians do NOT trigger leech from their own damage.
 */

#include "ScriptMgr.h"
#include "ScriptDefines/UnitScript.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "Config.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"

constexpr uint32 DEFAULT_HEAL_SPELL_ID = 81009;
constexpr uint32 DEFAULT_MANA_SPELL_ID = 81012;
constexpr uint32 FALLBACK_HEAL_SPELL_ID = 15286;
constexpr uint32 FALLBACK_MANA_SPELL_ID = 57669;

struct SoloSustainConfig
{
    bool enabled = false;
    uint32 healSpellId = DEFAULT_HEAL_SPELL_ID;
    uint32 manaSpellId = DEFAULT_MANA_SPELL_ID;
    bool petEnabled = true;
    bool guardianEnabled = true;
    float petMultiplier[MAX_CLASSES] = {};      // Per-class pet heal multiplier
    float guardianMultiplier[MAX_CLASSES] = {}; // Per-class guardian heal multiplier
    float lifeLeech[MAX_CLASSES] = {};
    float manaLeech[MAX_CLASSES] = {};
};

static SoloSustainConfig _config;

class SoloSustain_UnitScript : public UnitScript
{
public:
    SoloSustain_UnitScript() : UnitScript("SoloSustain_UnitScript", true, { UNITHOOK_ON_DAMAGE }) {}

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!_config.enabled || !attacker || !victim || victim == attacker || damage == 0)
            return;
        
        // ONLY player damage triggers leech (not pet/guardian damage)
        Player* player = attacker->ToPlayer();
        if (!player || !player->IsAlive())
            return;
        
        uint8 playerClass = player->GetClass();
        if (playerClass >= MAX_CLASSES)
            return;
        
        float lifeLeechPct = _config.lifeLeech[playerClass];
        float manaLeechPct = _config.manaLeech[playerClass];
        
        if (lifeLeechPct <= 0.0f && manaLeechPct <= 0.0f)
            return;
        
        uint32 healAmount = static_cast<uint32>(damage * lifeLeechPct);
        uint32 manaAmount = static_cast<uint32>(damage * manaLeechPct);
        
        SpellInfo const* healSpellInfo = sSpellMgr->GetSpellInfo(_config.healSpellId);
        if (!healSpellInfo)
            healSpellInfo = sSpellMgr->GetSpellInfo(FALLBACK_HEAL_SPELL_ID);
        
        // === HEAL PLAYER ===
        if (healAmount > 0 && healSpellInfo)
        {
            HealInfo hinfo(player, player, healAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
            player->HealBySpell(hinfo);
        }
        
        // === RESTORE PLAYER MANA ===
        if (manaAmount > 0 && player->GetMaxPower(POWER_MANA) > 0)
        {
            player->EnergizeBySpell(player, _config.manaSpellId, manaAmount, POWER_MANA);
        }
        
        // === HEAL PET (if enabled) ===
        if (_config.petEnabled)
        {
            if (Pet* pet = player->GetPet())
            {
                if (pet->IsAlive())
                {
                    float petMult = _config.petMultiplier[playerClass];
                    uint32 petHealAmount = static_cast<uint32>(healAmount * petMult);
                    uint32 petManaAmount = static_cast<uint32>(manaAmount * petMult);
                    
                    // Heal pet health
                    if (petHealAmount > 0 && healSpellInfo)
                    {
                        HealInfo petHinfo(pet, pet, petHealAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
                        pet->HealBySpell(petHinfo);
                    }
                    
                    // Restore pet mana (if pet uses mana)
                    if (petManaAmount > 0 && pet->GetMaxPower(POWER_MANA) > 0)
                    {
                        pet->EnergizeBySpell(pet, _config.manaSpellId, petManaAmount, POWER_MANA);
                    }
                }
            }
        }
        
        // === HEAL GUARDIANS (if enabled) ===
        if (_config.guardianEnabled && !player->m_Controlled.empty())
        {
            float guardMult = _config.guardianMultiplier[playerClass];
            uint32 guardHealAmount = static_cast<uint32>(healAmount * guardMult);
            uint32 guardManaAmount = static_cast<uint32>(manaAmount * guardMult);
            
            for (Unit::ControlSet::const_iterator itr = player->m_Controlled.begin(); itr != player->m_Controlled.end(); ++itr)
            {
                Unit* controlled = *itr;
                if (!controlled || !controlled->IsAlive())
                    continue;
                
                // Only heal guardians (not pets - already handled above)
                if (!controlled->IsGuardian())
                    continue;
                
                // Heal guardian health
                if (guardHealAmount > 0 && healSpellInfo)
                {
                    HealInfo guardianHinfo(controlled, controlled, guardHealAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
                    controlled->HealBySpell(guardianHinfo);
                }
                
                // Restore guardian mana (if guardian uses mana)
                if (guardManaAmount > 0 && controlled->GetMaxPower(POWER_MANA) > 0)
                {
                    controlled->EnergizeBySpell(controlled, _config.manaSpellId, guardManaAmount, POWER_MANA);
                }
            }
        }
    }
};

class SoloSustain_WorldScript : public WorldScript
{
public:
    SoloSustain_WorldScript() : WorldScript("SoloSustain_WorldScript") {}

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        _config.enabled = sConfigMgr->GetOption<bool>("SoloSustain.Enable", true);
        _config.healSpellId = sConfigMgr->GetOption<uint32>("SoloSustain.SpellId.Heal", DEFAULT_HEAL_SPELL_ID);
        _config.manaSpellId = sConfigMgr->GetOption<uint32>("SoloSustain.SpellId.Mana", DEFAULT_MANA_SPELL_ID);
        _config.petEnabled = sConfigMgr->GetOption<bool>("SoloSustain.Pet.Enable", true);
        _config.guardianEnabled = sConfigMgr->GetOption<bool>("SoloSustain.Guardian.Enable", true);
        
        _config.lifeLeech[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Warrior", 0.10f);
        _config.lifeLeech[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Paladin", 0.10f);
        _config.lifeLeech[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Hunter", 0.10f);
        _config.lifeLeech[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Rogue", 0.10f);
        _config.lifeLeech[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Priest", 0.10f);
        _config.lifeLeech[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.DeathKnight", 0.10f);
        _config.lifeLeech[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Shaman", 0.10f);
        _config.lifeLeech[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Mage", 0.10f);
        _config.lifeLeech[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Warlock", 0.10f);
        _config.lifeLeech[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Druid", 0.10f);
        
        _config.manaLeech[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Warrior", 0.0f);
        _config.manaLeech[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Paladin", 0.10f);
        _config.manaLeech[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Hunter", 0.10f);
        _config.manaLeech[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Rogue", 0.0f);
        _config.manaLeech[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Priest", 0.10f);
        _config.manaLeech[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.DeathKnight", 0.0f);
        _config.manaLeech[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Shaman", 0.10f);
        _config.manaLeech[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Mage", 0.10f);
        _config.manaLeech[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Warlock", 0.10f);
        _config.manaLeech[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Druid", 0.10f);
        
        // Pet heal multiplier per class (applied on top of life/mana leech)
        _config.petMultiplier[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Warrior", 1.0f);
        _config.petMultiplier[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Paladin", 1.0f);
        _config.petMultiplier[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Hunter", 1.0f);
        _config.petMultiplier[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Rogue", 1.0f);
        _config.petMultiplier[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Priest", 1.0f);
        _config.petMultiplier[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.DeathKnight", 1.0f);
        _config.petMultiplier[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Shaman", 1.0f);
        _config.petMultiplier[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Mage", 1.0f);
        _config.petMultiplier[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Warlock", 1.0f);
        _config.petMultiplier[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.Pet.Multiplier.Druid", 1.0f);
        
        // Guardian heal multiplier per class (DK ghouls, Shaman totems, etc.)
        _config.guardianMultiplier[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Warrior", 1.0f);
        _config.guardianMultiplier[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Paladin", 1.0f);
        _config.guardianMultiplier[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Hunter", 1.0f);
        _config.guardianMultiplier[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Rogue", 1.0f);
        _config.guardianMultiplier[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Priest", 1.0f);
        _config.guardianMultiplier[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.DeathKnight", 1.0f);
        _config.guardianMultiplier[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Shaman", 1.0f);
        _config.guardianMultiplier[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Mage", 1.0f);
        _config.guardianMultiplier[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Warlock", 1.0f);
        _config.guardianMultiplier[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Multiplier.Druid", 1.0f);
        
        if (_config.enabled)
        {
            LOG_INFO("module.solo_sustain", ">> Solo Sustain: Active (pet: {}, guardian: {})",
                _config.petEnabled ? "yes" : "no",
                _config.guardianEnabled ? "yes" : "no");
        }
    }
};

void Addmod_solo_sustainScripts()
{
    new SoloSustain_WorldScript();
    new SoloSustain_UnitScript();
}
