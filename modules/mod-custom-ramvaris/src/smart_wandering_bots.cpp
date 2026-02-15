/**
 * Smart Wandering Bots — zone-aware dynamic bot spawning
 *
 * Instead of spawning N bots globally across all continents (which loads every
 * grid they touch and tanks server performance), this system spawns a small number
 * of bots ONLY in zones that have players. Bots are SHARED per zone — if two
 * players are in the same zone, they see the SAME bots (not double).
 *
 * HOW IT WORKS:
 *   1. Set NpcBot.WanderingBots.Continents.Count = 0 in npcbots.conf (disable global spawn)
 *   2. On login / zone change, the system checks if the zone already has smart bots
 *   3. If no bots exist in the zone, MinAmount–MaxAmount bots are spawned
 *   4. If the zone already has bots (another player is there), no extra bots spawn
 *   5. When the LAST player leaves a zone, the zone's bots are despawned
 *   6. A cooldown prevents zone-border flapping from rapid spawn/despawn cycles
 *
 * ZONE-SHARED TRACKING:
 *   Bots are tracked per ZONE, not per player. A zone has a reference count of
 *   how many players are currently in it. Bots are spawned once when the first
 *   player enters and despawned when the last player leaves.
 *
 * FACTION BALANCING:
 *   When BalancedFaction.Enable = 1, bots are spawned with an equal 3-way split:
 *   Alliance, Horde, and Neutral each get ~1/3 of the spawns. This uses a
 *   round-robin approach that guarantees representation of all three factions
 *   even at very low counts (1-2 bots).
 *
 * CONFIG (in custom_ramvaris.conf.dist):
 *   CustomRamvaris.SmartWanderingBots.Enable                   = 0
 *   CustomRamvaris.SmartWanderingBots.MinAmount                = 1
 *   CustomRamvaris.SmartWanderingBots.MaxAmount                = 2
 *   CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown       = 30
 *   CustomRamvaris.SmartWanderingBots.BalancedFaction.Enable   = 1
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "GameTime.h"
#include "Log.h"
#include "botdatamgr.h"
#include "Map.h"

/// Per-zone shared bot tracking
struct SmartBotsZoneState
{
    std::vector<uint32>       spawnedEntries;   // Bot entries active in this zone
    std::set<ObjectGuid>      players;          // Players currently in this zone
};

/// Per-player lightweight tracking (cooldown + current zone)
struct SmartBotsPlayerState
{
    uint32 currentZoneId  = 0;
    uint32 lastZoneChange = 0;   // Cooldown timestamp (seconds since epoch)
};

class SmartWanderingBots_PlayerScript : public PlayerScript
{
public:
    SmartWanderingBots_PlayerScript() : PlayerScript("SmartWanderingBots_PlayerScript") {}

    void OnPlayerLogin(Player* player) override
    {
        if (!IsEnabled())
            return;

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
        auto pit = _playerStates.find(guid);
        if (pit != _playerStates.end())
        {
            uint32 oldZone = pit->second.currentZoneId;
            if (oldZone)
                RemovePlayerFromZone(guid, oldZone);
            _playerStates.erase(pit);
        }
    }

private:
    std::unordered_map<uint32, SmartBotsZoneState>         _zoneStates;    // keyed by zoneId
    std::unordered_map<ObjectGuid, SmartBotsPlayerState>   _playerStates;

    static bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true)
            && sConfigMgr->GetOption<bool>("CustomRamvaris.SmartWanderingBots.Enable", false);
    }

    static bool IsBalancedFactionEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.SmartWanderingBots.BalancedFaction.Enable", false);
    }

    // =========================================================================
    //  Core logic: player enters/leaves a zone
    // =========================================================================

    void HandleZoneChange(Player* player, uint32 newZone)
    {
        ObjectGuid guid = player->GetGUID();
        auto& pState = _playerStates[guid];

        // Already tracked in this zone — nothing to do
        if (pState.currentZoneId == newZone)
            return;

        // Cooldown check — prevent zone-border flapping
        uint32 cooldownSec = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown", 30);
        uint32 now = static_cast<uint32>(GameTime::GetGameTime().count());
        if (pState.lastZoneChange > 0 && (now - pState.lastZoneChange) < cooldownSec)
            return;
        pState.lastZoneChange = now;

        // Leave old zone (decrement refcount, despawn if last player)
        uint32 oldZone = pState.currentZoneId;
        if (oldZone)
            RemovePlayerFromZone(guid, oldZone);

        // Enter new zone (increment refcount, spawn if first player)
        pState.currentZoneId = newZone;
        AddPlayerToZone(player, guid, newZone);
    }

    /// Register player in a zone. Spawns bots if this is the first player entering.
    void AddPlayerToZone(Player* player, ObjectGuid guid, uint32 zoneId)
    {
        auto& zState = _zoneStates[zoneId];
        zState.players.insert(guid);

        // If bots already exist in this zone (another player was here first), just join
        if (!zState.spawnedEntries.empty())
        {
            BOT_LOG_INFO("module", "SmartWandering: {} joined zone {} — {} bots already active ({} players now)",
                player->GetName(), zoneId, zState.spawnedEntries.size(), zState.players.size());
            return;
        }

        // First player in this zone — spawn bots
        SpawnBotsForZone(player, zoneId, zState);
    }

    /// Unregister player from a zone. Despawns bots if this was the last player.
    void RemovePlayerFromZone(ObjectGuid guid, uint32 zoneId)
    {
        auto zit = _zoneStates.find(zoneId);
        if (zit == _zoneStates.end())
            return;

        auto& zState = zit->second;
        zState.players.erase(guid);

        if (zState.players.empty())
        {
            // Last player left — despawn all bots in this zone
            for (uint32 entry : zState.spawnedEntries)
                BotDataMgr::DespawnWandererBot(entry);

            BOT_LOG_INFO("module", "SmartWandering: All players left zone {} — despawned {} bots",
                zoneId, zState.spawnedEntries.size());

            _zoneStates.erase(zit);
        }
    }

    // =========================================================================
    //  Spawn logic
    // =========================================================================

    void SpawnBotsForZone(Player* player, uint32 zoneId, SmartBotsZoneState& zState)
    {
        uint32 minCount = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.MinAmount", 1);
        uint32 maxCount = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.MaxAmount", 2);
        uint32 count    = urand(minCount, maxCount);

        // Cap by spare pool
        uint32 available = BotDataMgr::GetAvailableWanderingBotCount();
        if (count > available)
            count = available;

        if (count == 0)
            return;

        uint32 mapId = player->GetMapId();
        std::vector<uint32> allEntries;
        allEntries.reserve(count);
        uint32 totalSpawned = 0;

        if (IsBalancedFactionEnabled())
        {
            // Equal 3-way round-robin: Alliance → Horde → Neutral → Alliance → ...
            // Each faction gets ~1/3 of the total spawns.
            static constexpr int32 factions[3] = { ALLIANCE, HORDE, TEAM_OTHER };
            uint32 facCounts[3] = { 0, 0, 0 };  // A, H, N spawned counts

            for (uint32 i = 0; i < count; ++i)
            {
                int32 faction = factions[i % 3];
                uint32 facIdx = i % 3;

                std::vector<uint32> entries;
                uint32 spawned = TrySpawnBot(zoneId, mapId, &entries, faction);

                // If primary faction fails, try the other two in order
                if (spawned == 0)
                {
                    for (uint32 retry = 1; retry <= 2 && spawned == 0; ++retry)
                    {
                        int32 altFaction = factions[(i + retry) % 3];
                        spawned = TrySpawnBot(zoneId, mapId, &entries, altFaction);
                        if (spawned > 0)
                            facIdx = (i + retry) % 3;
                    }
                }

                // Final fallback: unfiltered
                if (spawned == 0)
                    spawned = TrySpawnBot(zoneId, mapId, &entries, -1);

                if (spawned > 0)
                {
                    facCounts[facIdx] += spawned;
                    totalSpawned += spawned;
                    allEntries.insert(allEntries.end(), entries.begin(), entries.end());
                }
            }

            if (totalSpawned > 0)
            {
                BOT_LOG_INFO("module", "SmartWandering: Spawned {} bots for zone {} / map {} (A:{} H:{} N:{})",
                    totalSpawned, zoneId, mapId, facCounts[0], facCounts[1], facCounts[2]);
            }
        }
        else
        {
            // Standard spawning: all factions, natural distribution from spare pool
            totalSpawned = TrySpawnBots(zoneId, mapId, count, &allEntries, -1);

            if (totalSpawned > 0)
            {
                BOT_LOG_INFO("module", "SmartWandering: Spawned {} bots for zone {} / map {}",
                    totalSpawned, zoneId, mapId);
            }
        }

        if (totalSpawned > 0)
            zState.spawnedEntries = std::move(allEntries);
    }

    // =========================================================================
    //  Helpers
    // =========================================================================

    /// Try to spawn 1 bot: zone first, then map fallback.
    static uint32 TrySpawnBot(uint32 zoneId, uint32 mapId, std::vector<uint32>* outEntries, int32 team)
    {
        uint32 spawned = BotDataMgr::SpawnWanderingBotsInZone(zoneId, 1, outEntries, team);
        if (spawned == 0)
            spawned = BotDataMgr::SpawnWanderingBotsOnMap(mapId, 1, outEntries, team);
        return spawned;
    }

    /// Try to spawn N bots: zone first, then map fallback for remainder.
    static uint32 TrySpawnBots(uint32 zoneId, uint32 mapId, uint32 count, std::vector<uint32>* outEntries, int32 team)
    {
        uint32 spawned = BotDataMgr::SpawnWanderingBotsInZone(zoneId, count, outEntries, team);
        if (spawned < count)
            spawned += BotDataMgr::SpawnWanderingBotsOnMap(mapId, count - spawned, outEntries, team);
        return spawned;
    }
};

void AddSC_smart_wandering_bots()
{
    new SmartWanderingBots_PlayerScript();
}