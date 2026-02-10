/**
 * Smart Wandering Bots — zone-aware dynamic bot spawning
 *
 * Instead of spawning N bots globally across all continents (which loads every
 * grid they touch and tanks server performance), this system spawns a small number
 * of bots ONLY in the player's current zone. When the player moves to a new zone,
 * old bots are despawned and fresh ones are created at WanderNodes in the new zone.
 *
 * HOW IT WORKS:
 *   1. Set NpcBot.WanderingBots.Continents.Count = 0 in npcbots.conf (disable global spawn)
 *   2. On login / zone change, the system spawns MinAmount–MaxAmount bots in the player's zone
 *   3. On zone change, old bots are despawned and new ones are spawned in the new zone
 *   4. A configurable cooldown prevents zone-border flapping from rapid spawn/despawn cycles
 *   5. Per-player tracking: each player has their own set of smart-spawned bots
 *
 * PERFORMANCE:
 *   - Only MinAmount–MaxAmount bots exist at any time (vs. hundreds globally)
 *   - Only the player's current zone has loaded grids with bots in them
 *   - DespawnWandererBot handles both live bots AND pending spawn queue entries
 *   - Spawn stagger (500ms per bot) is handled by the existing BotDataMgr::Update() loop
 *
 * CONFIG (in custom_ramvaris.conf.dist):
 *   CustomRamvaris.SmartWanderingBots.Enable           = 0
 *   CustomRamvaris.SmartWanderingBots.MinAmount         = 5
 *   CustomRamvaris.SmartWanderingBots.MaxAmount         = 15
 *   CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown = 30
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "GameTime.h"
#include "Log.h"
#include "botdatamgr.h"

/// Per-player state for smart wandering bot tracking
struct SmartBotsPlayerState
{
    std::vector<uint32> spawnedEntries;       // Bot entries we spawned for this player
    uint32              currentZoneId = 0;    // Zone where bots are currently active
    uint32              lastZoneChange = 0;   // Cooldown timestamp (seconds since epoch)
};

class SmartWanderingBots_PlayerScript : public PlayerScript
{
public:
    SmartWanderingBots_PlayerScript() : PlayerScript("SmartWanderingBots_PlayerScript") {}

    void OnPlayerLogin(Player* player) override
    {
        if (!IsEnabled())
            return;

        // Trigger initial zone spawn on login — OnPlayerUpdateZone may not fire
        // for the first zone assignment
        uint32 zoneId, areaId;
        player->GetZoneAndAreaId(zoneId, areaId);
        HandleZoneChange(player, zoneId);
    }

    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 /*newArea*/) override
    {
        if (!IsEnabled())
            return;

        HandleZoneChange(player, newZone);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (!IsEnabled())
            return;

        ObjectGuid guid = player->GetGUID();
        DespawnPlayerBots(guid);
        _states.erase(guid);
    }

private:
    std::unordered_map<ObjectGuid, SmartBotsPlayerState> _states;

    static bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", false)
            && sConfigMgr->GetOption<bool>("CustomRamvaris.SmartWanderingBots.Enable", false);
    }

    void HandleZoneChange(Player* player, uint32 newZone)
    {
        ObjectGuid guid = player->GetGUID();
        auto& state = _states[guid];

        // Already have bots in this zone — nothing to do
        if (state.currentZoneId == newZone && !state.spawnedEntries.empty())
            return;

        // Cooldown check — prevent zone-border flapping
        uint32 cooldownSec = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown", 30);
        uint32 now = static_cast<uint32>(GameTime::GetGameTime().count());
        if (state.lastZoneChange > 0 && (now - state.lastZoneChange) < cooldownSec)
            return;
        state.lastZoneChange = now;

        // Despawn old bots from previous zone
        DespawnPlayerBots(guid);

        // Determine spawn count
        uint32 minCount = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.MinAmount", 5);
        uint32 maxCount = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.MaxAmount", 15);
        uint32 count = urand(minCount, maxCount);

        // Cap by spare pool — don't try to spawn more than available
        uint32 available = BotDataMgr::GetAvailableWanderingBotCount();
        if (count > available)
            count = available;

        if (count == 0)
        {
            BOT_LOG_DEBUG("module", "SmartWandering: No spare bots available for zone {} (player {})",
                newZone, player->GetName());
            return;
        }

        // Spawn new bots in the target zone
        std::vector<uint32> entries;
        uint32 spawned = BotDataMgr::SpawnWanderingBotsInZone(newZone, count, &entries);

        if (spawned > 0)
        {
            state.spawnedEntries = std::move(entries);
            state.currentZoneId = newZone;
            BOT_LOG_INFO("module", "SmartWandering: Spawned {} bots in zone {} for player {}",
                spawned, newZone, player->GetName());
        }
        else
        {
            // Zone might have no WanderNodes — that's fine (cities, instances)
            BOT_LOG_DEBUG("module", "SmartWandering: Zone {} has no valid WanderNodes for spawning", newZone);
        }
    }

    void DespawnPlayerBots(ObjectGuid guid)
    {
        auto it = _states.find(guid);
        if (it == _states.end())
            return;

        for (uint32 entry : it->second.spawnedEntries)
            BotDataMgr::DespawnWandererBot(entry);

        it->second.spawnedEntries.clear();
        it->second.currentZoneId = 0;
    }
};

void AddSC_smart_wandering_bots()
{
    new SmartWanderingBots_PlayerScript();
}
