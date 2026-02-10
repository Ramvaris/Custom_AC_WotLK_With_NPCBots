/*
 * Solo Sustain Module
 * Author: Ramvaris
 * 
 * Passive life and mana leech based on damage dealt by player AND their minions.
 * ALL damage sources sustain the player: direct hits, DoT ticks, pet melee/spells.
 * Player's pets and guardians also receive healing from the leech.
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

struct SoloSustainConfig
{
    bool enabled = false;
    uint32 healSpellId = DEFAULT_HEAL_SPELL_ID;
    uint32 manaSpellId = DEFAULT_MANA_SPELL_ID;
    bool petEnabled = true;
    bool guardianEnabled = true;
    float petHealthMultiplier[MAX_CLASSES] = {};      // Per-class pet HEALTH multiplier
    float petManaMultiplier[MAX_CLASSES] = {};        // Per-class pet MANA multiplier
    float guardianHealthMultiplier[MAX_CLASSES] = {}; // Per-class guardian HEALTH multiplier
    float guardianManaMultiplier[MAX_CLASSES] = {};   // Per-class guardian MANA multiplier
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
        
        // Resolve the actual player: For direct damage, attacker IS the player.
        // For pet/guardian damage, attacker is the pet — resolve via GetOwner().
        // For DoT/HoT ticks, attacker is always the caster (player).
        Player* player = attacker->ToPlayer();
        if (!player)
        {
            // Pet/Guardian damage: resolve owner for pet sustain
            if (Unit* owner = attacker->GetOwner())
                player = owner->ToPlayer();
        }
        if (!player || !player->IsAlive())
            return;
        
        // === BOT FILTERING ===
        // CRITICAL PERFORMANCE FIX: Skip NPC bots entirely!
        // With 100+ bots fighting, OnDamage fires THOUSANDS of times per second.
        // Each call = HealBySpell × 3 + pet/guardian iteration = massive overhead.
        // Only process REAL players to prevent server death spiral.
        if (player->IsNPCBot())
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
        // Uses HealBySpell/EnergizeBySpell for proper combat log + scrolling battle text.
        if (_config.petEnabled)
        {
            if (Pet* pet = player->GetPet())
            {
                if (pet->IsAlive())
                {
                    float petHealthMult = _config.petHealthMultiplier[playerClass];
                    float petManaMult = _config.petManaMultiplier[playerClass];
                    uint32 petHealAmount = static_cast<uint32>(healAmount * petHealthMult);
                    uint32 petManaAmount = static_cast<uint32>(manaAmount * petManaMult);
                    
                    if (petHealAmount > 0 && healSpellInfo)
                    {
                        HealInfo petHeal(pet, pet, petHealAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
                        pet->HealBySpell(petHeal);
                    }
                    
                    if (petManaAmount > 0 && pet->GetMaxPower(POWER_MANA) > 0)
                    {
                        pet->EnergizeBySpell(pet, _config.manaSpellId, petManaAmount, POWER_MANA);
                    }
                }
            }
        }
        
        // === HEAL GUARDIANS (if enabled) ===
        // Uses HealBySpell/EnergizeBySpell for proper combat log + scrolling battle text.
        if (_config.guardianEnabled && !player->m_Controlled.empty())
        {
            float guardHealthMult = _config.guardianHealthMultiplier[playerClass];
            float guardManaMult = _config.guardianManaMultiplier[playerClass];
            uint32 guardHealAmount = static_cast<uint32>(healAmount * guardHealthMult);
            uint32 guardManaAmount = static_cast<uint32>(manaAmount * guardManaMult);
            
            for (Unit::ControlSet::const_iterator itr = player->m_Controlled.begin(); itr != player->m_Controlled.end(); ++itr)
            {
                Unit* controlled = *itr;
                if (!controlled || !controlled->IsAlive())
                    continue;
                
                if (!controlled->IsGuardian())
                    continue;
                
                if (guardHealAmount > 0 && healSpellInfo)
                {
                    HealInfo guardHeal(controlled, controlled, guardHealAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
                    controlled->HealBySpell(guardHeal);
                }
                
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
        
        // Pet HEALTH multiplier per class
        _config.petHealthMultiplier[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Warrior", 1.0f);
        _config.petHealthMultiplier[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Paladin", 1.0f);
        _config.petHealthMultiplier[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Hunter", 1.0f);
        _config.petHealthMultiplier[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Rogue", 1.0f);
        _config.petHealthMultiplier[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Priest", 1.0f);
        _config.petHealthMultiplier[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.DeathKnight", 1.0f);
        _config.petHealthMultiplier[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Shaman", 1.0f);
        _config.petHealthMultiplier[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Mage", 1.0f);
        _config.petHealthMultiplier[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Warlock", 1.0f);
        _config.petHealthMultiplier[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.Pet.Health.Multiplier.Druid", 1.0f);
        
        // Pet MANA multiplier per class
        _config.petManaMultiplier[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Warrior", 1.0f);
        _config.petManaMultiplier[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Paladin", 1.0f);
        _config.petManaMultiplier[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Hunter", 1.0f);
        _config.petManaMultiplier[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Rogue", 1.0f);
        _config.petManaMultiplier[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Priest", 1.0f);
        _config.petManaMultiplier[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.DeathKnight", 1.0f);
        _config.petManaMultiplier[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Shaman", 1.0f);
        _config.petManaMultiplier[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Mage", 1.0f);
        _config.petManaMultiplier[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Warlock", 1.0f);
        _config.petManaMultiplier[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.Pet.Mana.Multiplier.Druid", 1.0f);
        
        // Guardian HEALTH multiplier per class
        _config.guardianHealthMultiplier[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Warrior", 1.0f);
        _config.guardianHealthMultiplier[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Paladin", 1.0f);
        _config.guardianHealthMultiplier[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Hunter", 1.0f);
        _config.guardianHealthMultiplier[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Rogue", 1.0f);
        _config.guardianHealthMultiplier[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Priest", 1.0f);
        _config.guardianHealthMultiplier[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.DeathKnight", 1.0f);
        _config.guardianHealthMultiplier[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Shaman", 1.0f);
        _config.guardianHealthMultiplier[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Mage", 1.0f);
        _config.guardianHealthMultiplier[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Warlock", 1.0f);
        _config.guardianHealthMultiplier[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Health.Multiplier.Druid", 1.0f);
        
        // Guardian MANA multiplier per class
        _config.guardianManaMultiplier[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Warrior", 1.0f);
        _config.guardianManaMultiplier[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Paladin", 1.0f);
        _config.guardianManaMultiplier[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Hunter", 1.0f);
        _config.guardianManaMultiplier[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Rogue", 1.0f);
        _config.guardianManaMultiplier[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Priest", 1.0f);
        _config.guardianManaMultiplier[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.DeathKnight", 1.0f);
        _config.guardianManaMultiplier[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Shaman", 1.0f);
        _config.guardianManaMultiplier[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Mage", 1.0f);
        _config.guardianManaMultiplier[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Warlock", 1.0f);
        _config.guardianManaMultiplier[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.Guardian.Mana.Multiplier.Druid", 1.0f);
        
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
