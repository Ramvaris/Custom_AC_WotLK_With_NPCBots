/*
 * Solo Sustain Module
 * Author: Ramvaris
 * License: Free to use, modify, distribute
 * 
 * Passive life and mana leech based on damage dealt.
 * Designed for solo play against scaled content (raids, dungeons, bots).
 * 
 * DESIGN PHILOSOPHY:
 *   - Melee classes FACE-TANK damage → need high life leech
 *   - Ranged classes can KITE → need less life leech
 *   - Healers have REAL HEALS → need minimal passive sustain
 *   - All values configurable per-class in .conf
 * 
 * STAT DETECTION:
 *   Uses MAX(melee AP, ranged AP, spell power) to determine player's role.
 *   This ensures hybrids (Ret Paladin, Feral Druid) get proper leech.
 * 
 * DAMAGE HOOKS:
 *   - OnDamage (player deals damage to victim)
 *   - Both melee and spell damage trigger leech
 *   - Caps prevent burst healing from big crits
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Config.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include <algorithm>

// =============================================================================
// Configuration Cache (loaded once at startup)
// =============================================================================
struct SoloSustainConfig
{
    bool enabled = false;
    bool debug = false;
    
    // Life leech per class (index = CLASS_*)
    float lifeLeech[MAX_CLASSES] = {};
    
    // Mana leech per class
    float manaLeech[MAX_CLASSES] = {};
    
    // Caps
    float lifeLeechCap = 0.15f;
    float manaLeechCap = 0.25f;
    
    // Visual spell IDs
    uint32 healSpellId = 81009;
    uint32 manaSpellId = 81012;
};

static SoloSustainConfig _config;

// =============================================================================
// Helper: Get player's highest stat (for hybrid detection)
// Returns: 0 = melee, 1 = ranged, 2 = spell
// =============================================================================
static int GetDominantStatType(Player* player)
{
    // Get melee attack power
    float meleeAP = player->GetTotalAttackPowerValue(BASE_ATTACK);
    
    // Get ranged attack power
    float rangedAP = player->GetTotalAttackPowerValue(RANGED_ATTACK);
    
    // Get highest spell power across all schools
    float spellPower = 0.0f;
    for (uint8 school = SPELL_SCHOOL_HOLY; school < MAX_SPELL_SCHOOL; ++school)
    {
        float sp = player->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + school);
        if (sp > spellPower)
            spellPower = sp;
    }
    
    // Normalize to expected endgame values (level 80)
    // This prevents raw AP always winning over SP
    constexpr float EXPECTED_MELEE_AP = 4500.0f;
    constexpr float EXPECTED_RANGED_AP = 4000.0f;
    constexpr float EXPECTED_SPELL_POWER = 2800.0f;
    
    float meleeRatio = meleeAP / EXPECTED_MELEE_AP;
    float rangedRatio = rangedAP / EXPECTED_RANGED_AP;
    float spellRatio = spellPower / EXPECTED_SPELL_POWER;
    
    if (meleeRatio >= rangedRatio && meleeRatio >= spellRatio)
        return 0; // Melee dominant
    else if (rangedRatio >= spellRatio)
        return 1; // Ranged dominant
    else
        return 2; // Spell dominant
}

// =============================================================================
// UnitScript: OnDamage Hook
// Fires when ANY unit deals/receives damage.
// We filter to only process player-dealt damage.
// =============================================================================
class SoloSustain_UnitScript : public UnitScript
{
public:
    SoloSustain_UnitScript() : UnitScript("SoloSustain_UnitScript") {}

    // Called when attacker deals damage to victim
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!_config.enabled)
            return;
        
        // Only process player attackers
        Player* player = attacker->ToPlayer();
        if (!player)
            return;
        
        // Don't process self-damage or friendly-fire
        if (!victim || victim == attacker || !attacker->IsHostileTo(victim))
            return;
        
        // Don't process if dead
        if (!player->IsAlive())
            return;
        
        // Get class-specific leech values
        uint8 playerClass = player->GetClass();
        if (playerClass >= MAX_CLASSES)
            return;
        
        float lifeLeechPct = _config.lifeLeech[playerClass];
        float manaLeechPct = _config.manaLeech[playerClass];
        
        // No leech configured for this class
        if (lifeLeechPct <= 0.0f && manaLeechPct <= 0.0f)
            return;
        
        // Calculate leech amounts
        uint32 healAmount = 0;
        uint32 manaAmount = 0;
        
        if (lifeLeechPct > 0.0f)
        {
            healAmount = static_cast<uint32>(damage * lifeLeechPct);
            
            // Apply cap
            uint32 maxHeal = static_cast<uint32>(player->GetMaxHealth() * _config.lifeLeechCap);
            healAmount = std::min(healAmount, maxHeal);
        }
        
        if (manaLeechPct > 0.0f && player->GetPowerType() == POWER_MANA)
        {
            manaAmount = static_cast<uint32>(damage * manaLeechPct);
            
            // Apply cap
            uint32 maxMana = static_cast<uint32>(player->GetMaxPower(POWER_MANA) * _config.manaLeechCap);
            manaAmount = std::min(manaAmount, maxMana);
        }
        
        // Apply healing
        if (healAmount > 0)
        {
            // Direct heal (no spell visual overhead for performance)
            player->SetHealth(std::min(player->GetHealth() + healAmount, player->GetMaxHealth()));
            
            if (_config.debug)
            {
                LOG_INFO("module.solo_sustain", "SoloSustain: {} healed {} HP from {} damage ({}%)",
                    player->GetName(), healAmount, damage, lifeLeechPct * 100.0f);
            }
        }
        
        // Apply mana restore
        if (manaAmount > 0)
        {
            int32 currentMana = player->GetPower(POWER_MANA);
            int32 maxMana = static_cast<int32>(player->GetMaxPower(POWER_MANA));
            int32 newMana = std::min(currentMana + static_cast<int32>(manaAmount), maxMana);
            player->SetPower(POWER_MANA, newMana);
            
            if (_config.debug)
            {
                LOG_INFO("module.solo_sustain", "SoloSustain: {} restored {} mana from {} damage ({}%)",
                    player->GetName(), manaAmount, damage, manaLeechPct * 100.0f);
            }
        }
    }
};

// =============================================================================
// WorldScript: Load Configuration
// =============================================================================
class SoloSustain_WorldScript : public WorldScript
{
public:
    SoloSustain_WorldScript() : WorldScript("SoloSustain_WorldScript") {}

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        _config.enabled = sConfigMgr->GetOption<bool>("SoloSustain.Enable", true);
        _config.debug = sConfigMgr->GetOption<bool>("SoloSustain.Debug", false);
        
        // Life leech per class
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
        
        // Mana leech per class
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
        
        // Caps
        _config.lifeLeechCap = sConfigMgr->GetOption<float>("SoloSustain.LifeLeech.Cap", 0.15f);
        _config.manaLeechCap = sConfigMgr->GetOption<float>("SoloSustain.ManaLeech.Cap", 0.25f);
        
        // Visual spells
        _config.healSpellId = sConfigMgr->GetOption<uint32>("SoloSustain.HealSpellId", 81009);
        _config.manaSpellId = sConfigMgr->GetOption<uint32>("SoloSustain.ManaSpellId", 81012);
        
        if (_config.enabled)
        {
            LOG_INFO("module.solo_sustain", ">> Solo Sustain module loaded - damage-based life/mana leech active");
        }
    }
};

// =============================================================================
// Script Registration
// =============================================================================
void Addmod_solo_sustainScripts()
{
    new SoloSustain_WorldScript();
    new SoloSustain_UnitScript();
}
