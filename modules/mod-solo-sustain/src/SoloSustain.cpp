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
 * STAT DETECTION:
 *   Uses MAX(melee AP, ranged AP, spell power) to determine player's role.
 *   This ensures hybrids (Ret Paladin, Feral Druid) get proper leech.
 * 
 * DAMAGE HOOKS:
 *   - OnDamage (player deals damage to victim)
 *   - Both melee and spell damage trigger leech
 *   - Caps prevent burst healing from big crits
 * 
 * COMBAT LOG VISIBILITY (Recount/MSBT):
 *   - Sends SMSG_SPELLHEALLOG for life leech (shows as "X heals Y for Z")
 *   - Sends SMSG_SPELLENERGIZELOG for mana leech (shows mana gain)
 *   - Uses standard client spells for compatibility:
 *     - 15290 (Vampiric Embrace) for life leech visual
 *     - 57669 (Replenishment) for mana leech visual
 */

#include "ScriptMgr.h"
#include "ScriptDefines/UnitScript.h"
#include "Player.h"
#include "Unit.h"
#include "Config.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include <algorithm>

// Standard client spell IDs for combat log visibility (no custom spells needed)
constexpr uint32 SPELL_VAMPIRIC_EMBRACE_HEAL = 15290;  // Shows as lifesteal in combat log
constexpr uint32 SPELL_REPLENISHMENT_MANA    = 57669;  // Shows as mana gain in combat log

// =============================================================================
// Configuration Cache (loaded once at startup)
// =============================================================================
struct SoloSustainConfig
{
    bool enabled = false;
    bool debug = false;
    bool showCombatLog = true;  // Send packets to combat log (Recount/MSBT)
    
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
        
        // Apply healing and send combat log packet (Recount/MSBT visibility)
        if (healAmount > 0)
        {
            // Calculate actual heal (prevent overheal in log)
            uint32 currentHealth = healTarget->GetHealth();
            uint32 maxHealth = healTarget->GetMaxHealth();
            uint32 actualHeal = std::min(healAmount, maxHealth - currentHealth);
            uint32 overheal = healAmount - actualHeal;
            
            // Apply the heal to the correct target (player OR pet/guardian)
            healTarget->SetHealth(currentHealth + actualHeal);
            
            // Send SMSG_SPELLHEALLOG for combat log visibility
            // This makes the heal show up in Recount, Details!, MSBT, etc.
            if (_config.showCombatLog && actualHeal > 0)
            {
                WorldPacket data(SMSG_SPELLHEALLOG, 8 + 8 + 4 + 4 + 4 + 1);
                data << healTarget->GetPackGUID();       // Target (player or pet)
                data << healTarget->GetPackGUID();       // Caster (self-heal)
                data << uint32(SPELL_VAMPIRIC_EMBRACE_HEAL);  // Spell ID (Vampiric Embrace - thematic!)
                data << uint32(actualHeal);              // Heal amount
                data << uint32(overheal);                // Overheal amount
                data << uint8(0);                        // Critical flag (0 = no crit)
                player->SendMessageToSet(&data, true);   // Send to player's network set
            }
            
            if (_config.debug)
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
        
        // Apply mana restore and send combat log packet (players only, pets don't get mana)
        if (manaAmount > 0)
        {
            int32 currentMana = player->GetPower(POWER_MANA);
            int32 maxMana = static_cast<int32>(player->GetMaxPower(POWER_MANA));
            int32 actualMana = std::min(static_cast<int32>(manaAmount), maxMana - currentMana);
            
            // Apply the mana
            player->SetPower(POWER_MANA, currentMana + actualMana);
            
            // Send SMSG_SPELLENERGIZELOG for combat log visibility
            if (_config.showCombatLog && actualMana > 0)
            {
                WorldPacket data(SMSG_SPELLENERGIZELOG, 8 + 8 + 4 + 4 + 4);
                data << player->GetPackGUID();           // Target
                data << player->GetPackGUID();           // Caster (self)
                data << uint32(SPELL_REPLENISHMENT_MANA);     // Spell ID (Replenishment - standard)
                data << uint32(POWER_MANA);              // Power type
                data << uint32(actualMana);              // Amount
                player->SendMessageToSet(&data, true);
            }
            
            if (_config.debug)
            {
                LOG_INFO("module.solo_sustain", "SoloSustain: {} restored {} mana from {} damage ({}%)",
                    player->GetName(), actualMana, damage, manaLeechPct * 100.0f);
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
        _config.showCombatLog = sConfigMgr->GetOption<bool>("SoloSustain.ShowCombatLog", true);
        
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
