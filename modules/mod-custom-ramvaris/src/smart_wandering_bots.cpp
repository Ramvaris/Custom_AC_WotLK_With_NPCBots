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
 * FACTION BALANCING:
 *   When BalancedFaction.Enable = 1, each bot individually has a 50/50 chance of
 *   being Alliance or Horde (coin-flip per bot). This gives natural faction mixing
 *   even at very low counts (1-2 bots) without the integer-division problems of
 *   splitting a count evenly.
 *
 * MULTI-PLAYER:
 *   Each player independently spawns their own set of bots in their current zone.
 *   Bots are tracked per-player via ObjectGuid. Logging out despawns only that
 *   player's bots. Zone changes only affect that player's bots.
 *
 * CONFIG (in custom_ramvaris.conf.dist):
 *   CustomRamvaris.SmartWanderingBots.Enable                   = 0
 *   CustomRamvaris.SmartWanderingBots.MinAmount                = 5
 *   CustomRamvaris.SmartWanderingBots.MaxAmount                = 15
 *   CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown       = 30
 *   CustomRamvaris.SmartWanderingBots.BalancedFaction.Enable   = 0
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "GameTime.h"
#include "Log.h"
#include "botdatamgr.h"
#include "Map.h"

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
        return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true)
            && sConfigMgr->GetOption<bool>("CustomRamvaris.SmartWanderingBots.Enable", false);
    }

    static bool IsBalancedFactionEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.SmartWanderingBots.BalancedFaction.Enable", false);
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
            // Coin-flip per bot: each bot individually gets a random faction.
            uint32 aSpawned = 0;
            uint32 hSpawned = 0;

            for (uint32 i = 0; i < count; ++i)
            {
                int32 faction = urand(0, 1) ? ALLIANCE : HORDE;

                std::vector<uint32> entries;
                uint32 spawned = TrySpawnBot(newZone, mapId, &entries, faction);

                if (spawned == 0)
                {
                    // Try the other faction
                    int32 otherFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;
                    spawned = TrySpawnBot(newZone, mapId, &entries, otherFaction);
                }

                if (spawned == 0)
                {
                    // Both factions failed — try unfiltered
                    spawned = TrySpawnBot(newZone, mapId, &entries, -1);
                }

                if (spawned > 0)
                {
                    if (faction == ALLIANCE)
                        aSpawned += spawned;
                    else
                        hSpawned += spawned;

                    totalSpawned += spawned;
                    allEntries.insert(allEntries.end(), entries.begin(), entries.end());
                }
            }

            if (totalSpawned > 0)
            {
                BOT_LOG_INFO("module", "SmartWandering: Spawned {} bots for {} in zone {} / map {} (A:{} H:{})",
                    totalSpawned, player->GetName(), newZone, mapId, aSpawned, hSpawned);
            }
        }
        else
        {
            // Standard spawning: all factions, natural distribution from spare pool
            totalSpawned = TrySpawnBots(newZone, mapId, count, &allEntries, -1);

            if (totalSpawned > 0)
            {
                BOT_LOG_INFO("module", "SmartWandering: Spawned {} bots for {} in zone {} / map {}",
                    totalSpawned, player->GetName(), newZone, mapId);
            }
        }

        if (totalSpawned > 0)
        {
            state.spawnedEntries = std::move(allEntries);
            state.currentZoneId = newZone;
        }
    }

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