/*
 * Solo Sustain Module
 * Author: Ramvaris
 * License: Free to use, modify, distribute as long as you mention the original author
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
 * DAMAGE HOOKS:
 *   - OnDamage (player deals damage to victim)
 *   - Both melee and spell damage trigger leech
 *   - Caps prevent burst healing from big crits
 * 
 * COMBAT LOG VISIBILITY (Recount/MSBT):
 *   - Uses Unit::HealBySpell() with proper HealInfo struct
 *   - Uses Unit::EnergizeBySpell() for mana restoration
 *   - These are the PROPER AC methods that trigger combat log events!
 *   - Standard 3.3.5a spell IDs from DBC for client compatibility
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

// =============================================================================
// Real WoW 3.3.5a Spell IDs (exist in DBC, used by AC internally)
// These trigger proper combat log events that Recount/MSBT can see!
// =============================================================================
// Life Steal heal - used by Lifestealing enchant and Judgement of Light
// Shows as "Life Steal" in combat log - PERFECT for vampire theme!
constexpr uint32 SPELL_LIFE_STEAL_HEAL = 20267;

// Mana restore - using the same spell for energize (shows as generic mana gain)
constexpr uint32 SPELL_MANA_RESTORE = 20268;  // Judgement of Wisdom energize

// =============================================================================
// Configuration Cache (loaded once at startup)
// =============================================================================
struct SoloSustainConfig
{
    bool enabled = false;
    bool debug = false;
    // NOTE: Combat log visibility is ALWAYS on - HealBySpell/EnergizeBySpell handle it
    
    // Pet/Guardian support
    bool petEnabled = true;           // Enable leech for player pets/guardians
    float petLeechMultiplier = 1.5f;  // Pets get MORE leech (squishier than players)
    
    // Life leech per class (index = CLASS_*)
    float lifeLeech[MAX_CLASSES] = {};
    
    // Mana leech per class
    float manaLeech[MAX_CLASSES] = {};
    
    // Caps
    float lifeLeechCap = 0.15f;
    float manaLeechCap = 0.25f;
};

static SoloSustainConfig _config;

// =============================================================================
// UnitScript: OnDamage Hook
// Fires when ANY unit deals/receives damage.
// Handles: Players directly, Pets/Guardians (heal themselves based on owner's class).
// CRITICAL: Must register UNITHOOK_ON_DAMAGE or the hook won't be called!
// =============================================================================
class SoloSustain_UnitScript : public UnitScript
{
public:
    SoloSustain_UnitScript() : UnitScript("SoloSustain_UnitScript", true, { UNITHOOK_ON_DAMAGE }) {}

    // Called when attacker deals damage to victim
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!_config.enabled)
            return;
        
        // Don't process self-damage or friendly-fire
        if (!victim || victim == attacker || !attacker->IsHostileTo(victim))
            return;
        
        // Determine if this is a player or a pet/guardian
        Player* player = attacker->ToPlayer();
        Unit* healTarget = attacker;  // Who gets healed (player or pet)
        float leechMultiplier = 1.0f;
        bool isPetOrGuardian = false;
        
        if (!player)
        {
            // Not a player - check if it's a pet or guardian owned by a player
            if (!_config.petEnabled)
                return;
            
            // IsPet() = permanent pets (Hunter, Warlock)
            // IsGuardian() = temporary summons (DK ghoul, Shaman wolves, etc.)
            if (!attacker->IsPet() && !attacker->IsGuardian())
                return;
            
            // Get the player owner
            Unit* owner = attacker->GetOwner();
            if (!owner)
                return;
            
            player = owner->ToPlayer();
            if (!player)
                return;
            
            isPetOrGuardian = true;
            leechMultiplier = _config.petLeechMultiplier;
            // healTarget stays as 'attacker' (the pet/guardian heals itself)
        }
        
        // Don't process if the healer is dead
        if (!healTarget->IsAlive())
            return;
        
        // Get class-specific leech values from the player (owner for pets)
        uint8 playerClass = player->GetClass();
        if (playerClass >= MAX_CLASSES)
            return;
        
        float lifeLeechPct = _config.lifeLeech[playerClass] * leechMultiplier;
        float manaLeechPct = _config.manaLeech[playerClass] * leechMultiplier;
        
        // No leech configured for this class
        if (lifeLeechPct <= 0.0f && manaLeechPct <= 0.0f)
            return;
        
        // Calculate leech amounts
        uint32 healAmount = 0;
        uint32 manaAmount = 0;
        
        if (lifeLeechPct > 0.0f)
        {
            healAmount = static_cast<uint32>(damage * lifeLeechPct);
            
            // Apply cap based on heal target's max health
            uint32 maxHeal = static_cast<uint32>(healTarget->GetMaxHealth() * _config.lifeLeechCap);
            healAmount = std::min(healAmount, maxHeal);
        }
        
        // Mana leech only for mana users (skip for most pets, they use focus/energy)
        // Players always get mana leech if they use mana
        // For pets: only if they actually use mana (Warlock Imp/Succubus/Felhunter)
        if (manaLeechPct > 0.0f && !isPetOrGuardian && player->GetPowerType() == POWER_MANA)
        {
            manaAmount = static_cast<uint32>(damage * manaLeechPct);
            
            // Apply cap
            uint32 maxMana = static_cast<uint32>(player->GetMaxPower(POWER_MANA) * _config.manaLeechCap);
            manaAmount = std::min(manaAmount, maxMana);
        }
        
        // =================================================================
        // Apply healing - NO spell casting, NO visual effects!
        // We use HealBySpell which:
        //   1. Applies health via DealHeal() 
        //   2. Sends SMSG_SPELLHEALLOG for combat log (Recount/MSBT sees it)
        //   3. Client shows green floating number from the log packet
        // The spell ID is ONLY for the combat log text - no animations!
        // =================================================================
        if (healAmount > 0)
        {
            // Get SpellInfo for Life Steal (20267) - just for combat log name!
            SpellInfo const* healSpellInfo = sSpellMgr->GetSpellInfo(SPELL_LIFE_STEAL_HEAL);
            if (healSpellInfo)
            {
                // Create HealInfo (healer = target = self-heal)
                HealInfo hinfo(healTarget, healTarget, healAmount, healSpellInfo, healSpellInfo->GetSchoolMask());
                
                // HealBySpell does NOT cast - no visual effects!
                // Just: DealHeal() + SendHealSpellLog()
                int32 actualHeal = healTarget->HealBySpell(hinfo);
                
                if (_config.debug && actualHeal > 0)
                {
                    if (isPetOrGuardian)
                    {
                        LOG_INFO("module.solo_sustain", "SoloSustain: {}'s pet healed {} HP from {} damage ({}% x {}x)",
                            player->GetName(), actualHeal, damage, _config.lifeLeech[playerClass] * 100.0f, leechMultiplier);
                    }
                    else
                    {
                        LOG_INFO("module.solo_sustain", "SoloSustain: {} healed {} HP from {} damage ({}%)",
                            player->GetName(), actualHeal, damage, lifeLeechPct * 100.0f);
                    }
                }
            }
        }
        
        // =================================================================
        // Apply mana - NO spell casting, NO visual effects!
        // EnergizeBySpell does NOT cast - just:
        //   1. ModifyPower() to add mana
        //   2. SendEnergizeSpellLog() for combat log (blue number)
        // =================================================================
        if (manaAmount > 0)
        {
            // EnergizeBySpell does NOT cast - no visual effects!
            player->EnergizeBySpell(player, SPELL_MANA_RESTORE, manaAmount, POWER_MANA);
            
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
        // NOTE: ShowCombatLog removed - HealBySpell/EnergizeBySpell ALWAYS trigger combat log
        
        // Pet/Guardian support
        _config.petEnabled = sConfigMgr->GetOption<bool>("SoloSustain.Pet.Enable", true);
        _config.petLeechMultiplier = sConfigMgr->GetOption<float>("SoloSustain.Pet.LeechMultiplier", 1.5f);
        
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
