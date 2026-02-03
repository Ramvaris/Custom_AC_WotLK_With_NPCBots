/*
 * Soul Keeper Module - Header
 * Author: Ramvaris
 * License: Free to use, modify, distribute as long as you mention the original author
 * 
 * Allows any class to capture creature souls and summon them as guardians
 * 
 * Architecture:
 * - PlayerScript: Handles gossip menu interactions (no NPC required)
 * - CommandScript: Handles .soul commands
 * - SoulKeeper Singleton: Core logic for capture, summon, dismiss, scaling
 */

#ifndef SOUL_KEEPER_H
#define SOUL_KEEPER_H

#include "Common.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Creature.h"
#include "Pet.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include <string>
#include <unordered_map>
#include <vector>

// Menu Constants
constexpr uint32 SOUL_KEEPER_GOSSIP_MENU_ID = 89999;
constexpr uint32 SOUL_KEEPER_NPC_TEXT_ID    = 0x7FFFFFFF;
constexpr uint32 SOUL_KEEPER_GOSSIP_SENDER  = 8999;  // Unique sender to avoid Lua gossip collisions
constexpr uint32 SOULS_PER_PAGE             = 20;   // Souls per page (keep under gossip cap with nav/sort)

// Soul Keeper Guardian Marker - set in UNIT_CREATED_BY_SPELL field
// Used in core's RemoveEvadeAuras to identify our guardians and skip aura removal
// This is just a marker value - no actual spell needs to exist!
constexpr uint32 SPELL_SOUL_KEEPER_GUARDIAN = 81100;

// Gossip Actions
enum SoulKeeperGossipAction
{
    SOUL_ACTION_CLOSE           = 0,
    SOUL_ACTION_PREV_PAGE       = 1,
    SOUL_ACTION_NEXT_PAGE       = 2,
    SOUL_ACTION_FIRST_PAGE      = 3,
    SOUL_ACTION_LAST_PAGE       = 4,
    SOUL_ACTION_SORT_NEWEST     = 10,
    SOUL_ACTION_SORT_OLDEST     = 11,
    SOUL_ACTION_SORT_ALPHA_ASC  = 12,
    SOUL_ACTION_SORT_ALPHA_DESC = 13,
    SOUL_ACTION_SUMMON_BASE     = 100,   // Actions 100+ = Summon by index
};

// Soul List Sort Modes (display order only - summon IDs remain capture-order)
enum SoulKeeperSortMode : uint8
{
    SOUL_SORT_NEWEST     = 0,
    SOUL_SORT_OLDEST     = 1,
    SOUL_SORT_ALPHA_ASC  = 2,
    SOUL_SORT_ALPHA_DESC = 3,
};

// Soul Data Structure
struct SoulData
{
    uint32 creatureEntry;
    std::string customName;
    uint32 displayId;
    float scaleFactor;
};

// Core Singleton
class SoulKeeper
{
public:
    static SoulKeeper* instance();

    // Data Storage: Map<PlayerGUIDLow, List<Souls>>
    std::unordered_map<uint32, std::vector<SoulData>> _caughtSouls;
    
    // Active Guardians: Map<PlayerGUIDLow, GuardianGUID>
    std::unordered_map<uint32, ObjectGuid> _activeGuardians;

    // Guardian Scaling Data: Map<GuardianGUID, ScalingInfo>
    // Stores pre-calculated gear ratio and owner level for proper damage scaling.
    // Used by UnitScript ModifySpellDamageTaken / ModifyPeriodicDamageAurasTick.
    // gearRatio is ALREADY NORMALIZED - spell hooks use it directly (no division).
    struct GuardianScalingInfo
    {
        float gearRatio;          // Pre-calculated gear quality ratio (0.5 minimum, scales up)
        uint32 creatureLevel;     // Original creature template level (for scaling direction)
        uint32 ownerLevel;        // Owner level at summon time
    };
    std::unordered_map<ObjectGuid, GuardianScalingInfo> _guardianScaling;

    // Guardian AI Timer: Map<GuardianGUID, NextAITickTime>
    // Per-guardian AI injection cooldown (not global singleton timer!)
    std::unordered_map<ObjectGuid, uint32> _guardianAITimer;

    // Guardian Damage Tracking: Map<GuardianGUID, LastDamageTimestamp>
    // Used to trigger defensive cooldowns (immunity buffs) only when ACTUALLY taking damage
    std::unordered_map<ObjectGuid, uint32> _guardianLastDamage;

    // Guardian Owner Damage Tracking: Map<GuardianGUID, LastOwnerDamageTimestamp>
    // Used to trigger immunities on the OWNER only when the owner actually took damage
    std::unordered_map<ObjectGuid, uint32> _guardianLastOwnerDamage;

    // Persistent Cooldown Tracking: Prevents dismiss/summon exploit to reset cooldowns!
    // Structure: ownerGUIDLow -> creatureEntry -> spellId -> cooldownEndTime (absolute getMSTime)
    // Cooldowns persist across summon/dismiss cycles per guardian TYPE per OWNER.
    // When guardian is dismissed: remaining cooldowns are saved here.
    // When guardian is summoned: cooldowns are restored from here.
    // RAM usage: ~20 bytes per active cooldown, acceptable tradeoff for exploit prevention.
    std::unordered_map<uint32, std::unordered_map<uint32, std::unordered_map<uint32, uint32>>> _persistentCooldowns;

    // Gossip Page Tracking: Map<PlayerGUIDLow, CurrentPage>
    std::unordered_map<uint32, uint32> _currentGossipPage;

    // Gossip Sort Tracking: Map<PlayerGUIDLow, SortMode>
    std::unordered_map<uint32, uint8> _currentSortMode;

    // Database Operations
    void LoadSouls(Player* player);
    void SaveSoul(Player* player, SoulData const& soul);
    bool HasSoul(Player* player, uint32 entry);

    // Core Logic
    void AddGuardian(Player* player, Unit* victim);
    void SummonGuardian(Player* player, uint32 entry);
    void DismissGuardian(Player* player);
    void ReturnGuardian(Player* player);  // .soul return - despawn and trigger cooldown
    void OnGuardianDeath(Creature* guardian);  // Called when guardian dies
    void RenameGuardian(Player* player, std::string const& newName);
    void ScaleGuardian(Creature* guardian, Player* owner);
    
    // Cooldown Persistence (prevent dismiss/summon exploit)
    void SaveGuardianCooldowns(Creature* guardian, Player* owner);
    void RestoreGuardianCooldowns(Creature* guardian, Player* owner);
    
    // Gossip Menu System (paginated for 400+ souls)
    void ShowSoulList(Player* player, uint32 page = 0);
    bool HandleGossipSelect(Player* player, uint32 sender, uint32 action);
    
    // Utility
    std::string GetCreatureIconString(uint32 displayId);
    bool IsInCombat(Player* player);
    bool HasActiveGuardian(Player* player);
};

#define sSoulKeeper SoulKeeper::instance()

#endif
