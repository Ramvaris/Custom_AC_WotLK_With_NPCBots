/*
 * Solo Sustain Module
 * Author: Ramvaris
 * 
 * Passive life and mana leech based on damage dealt.
 * Designed for solo play against scaled content (raids, dungeons, bots).
 * 
 * Supports: Players and their permanent pets (Hunter/Warlock IsPet()).
 * Does NOT support: Guardians (temporary summons) - they use creature AI.
 */

#include "ScriptMgr.h"
#include "ScriptDefines/UnitScript.h"
#include "Player.h"
#include "Unit.h"
#include "Config.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include <algorithm>

constexpr uint32 DEFAULT_HEAL_SPELL_ID = 81009;   // Custom dummy heal spell
constexpr uint32 DEFAULT_MANA_SPELL_ID = 81012;   // Custom dummy mana restore spell
constexpr uint32 FALLBACK_HEAL_SPELL_ID = 15286;  // Vampiric Embrace (exists in all clients)
constexpr uint32 FALLBACK_MANA_SPELL_ID = 57669;  // Replenishment (exists in all clients)

struct SoloSustainConfig
{
    bool enabled = false;
    bool debug = false;
    uint32 healSpellId = DEFAULT_HEAL_SPELL_ID;
    uint32 manaSpellId = DEFAULT_MANA_SPELL_ID;
    bool petEnabled = true;
    float petLeechMultiplier = 1.5f;
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
        
        Player* player = attacker->ToPlayer();
        Unit* healTarget = attacker;
        float leechMultiplier = 1.0f;
        bool isPet = false;
        
        if (!player)
        {
            // Only support permanent pets (Hunter/Warlock) - NOT guardians
            if (!_config.petEnabled || !attacker->IsPet())
                return;
            
            Unit* owner = attacker->GetOwner();
            if (!owner)
                return;
            
            player = owner->ToPlayer();
            if (!player)
                return;
            
            isPet = true;
            leechMultiplier = _config.petLeechMultiplier;
        }
        
        if (!healTarget->IsAlive())
            return;
        
        uint8 playerClass = player->GetClass();
        if (playerClass >= MAX_CLASSES)
            return;
        
        float lifeLeechPct = _config.lifeLeech[playerClass] * leechMultiplier;
        float manaLeechPct = _config.manaLeech[playerClass] * leechMultiplier;
        
        if (lifeLeechPct <= 0.0f && manaLeechPct <= 0.0f)
            return;
        
        // Life leech (no cap - damage-based leech is self-limiting)
        if (lifeLeechPct > 0.0f)
        {
            uint32 healAmount = static_cast<uint32>(damage * lifeLeechPct);
            
            if (healAmount > 0)
            {
                SpellInfo const* healSpellInfo = sSpellMgr->GetSpellInfo(_config.healSpellId);
                if (!healSpellInfo)
                {
                    // Configured spell doesn't exist - use standard WoW fallback
                    healSpellInfo = sSpellMgr->GetSpellInfo(FALLBACK_HEAL_SPELL_ID);
                }
                
                if (healSpellInfo)
                {
                    HealInfo hinfo(healTarget, healTarget, healAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
                    healTarget->HealBySpell(hinfo);
                    
    
                }
                else
                {
                    LOG_ERROR("module.solo_sustain", "SoloSustain: No valid heal spell found! Config: {}, Fallback: {}",
                        _config.healSpellId, FALLBACK_HEAL_SPELL_ID);
                }
            }
        }
        
        // Mana leech (players only, not pets - no cap needed for damage-based)
        if (manaLeechPct > 0.0f && !isPet && player->GetMaxPower(POWER_MANA) > 0)
        {
            uint32 manaAmount = static_cast<uint32>(damage * manaLeechPct);
            
            if (manaAmount > 0)
            {
                player->EnergizeBySpell(player, _config.manaSpellId, manaAmount, POWER_MANA);
                

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
        _config.debug = sConfigMgr->GetOption<bool>("SoloSustain.Debug", false);
        _config.healSpellId = sConfigMgr->GetOption<uint32>("SoloSustain.SpellId.Heal", DEFAULT_HEAL_SPELL_ID);
        _config.manaSpellId = sConfigMgr->GetOption<uint32>("SoloSustain.SpellId.Mana", DEFAULT_MANA_SPELL_ID);
        _config.petEnabled = sConfigMgr->GetOption<bool>("SoloSustain.Pet.Enable", true);
        _config.petLeechMultiplier = sConfigMgr->GetOption<float>("SoloSustain.Pet.LeechMultiplier", 1.5f);
        
        _config.lifeLeech[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Warrior", 0.08f);
        _config.lifeLeech[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Paladin", 0.02f);
        _config.lifeLeech[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Hunter", 0.04f);
        _config.lifeLeech[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Rogue", 0.08f);
        _config.lifeLeech[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Priest", 0.02f);
        _config.lifeLeech[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.DeathKnight", 0.05f);
        _config.lifeLeech[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Shaman", 0.02f);
        _config.lifeLeech[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Mage", 0.03f);
        _config.lifeLeech[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Warlock", 0.02f);
        _config.lifeLeech[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Druid", 0.02f);
        
        _config.manaLeech[CLASS_WARRIOR]      = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Warrior", 0.0f);
        _config.manaLeech[CLASS_PALADIN]      = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Paladin", 0.08f);
        _config.manaLeech[CLASS_HUNTER]       = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Hunter", 0.08f);
        _config.manaLeech[CLASS_ROGUE]        = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Rogue", 0.0f);
        _config.manaLeech[CLASS_PRIEST]       = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Priest", 0.08f);
        _config.manaLeech[CLASS_DEATH_KNIGHT] = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.DeathKnight", 0.0f);
        _config.manaLeech[CLASS_SHAMAN]       = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Shaman", 0.08f);
        _config.manaLeech[CLASS_MAGE]         = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Mage", 0.10f);
        _config.manaLeech[CLASS_WARLOCK]      = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Warlock", 0.08f);
        _config.manaLeech[CLASS_DRUID]        = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Druid", 0.08f);
        
        if (_config.enabled)
        {
            LOG_INFO("module.solo_sustain", ">> Solo Sustain: Active (pets: {})",
                _config.petEnabled ? "enabled" : "disabled");
        }
    }
};

void Addmod_solo_sustainScripts()
{
    new SoloSustain_WorldScript();
    new SoloSustain_UnitScript();
}
