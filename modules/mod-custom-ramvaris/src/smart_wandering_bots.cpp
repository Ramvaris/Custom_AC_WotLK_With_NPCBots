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
 *   When BalancedFaction.Enable = 1, the total spawn count is split evenly between
 *   Alliance and Horde. Each half is spawned separately using faction-specific spare
 *   pools. If one faction has no available bots or no valid spawn nodes in the zone,
 *   the remaining count goes to the other faction. This ensures zones aren't dominated
 *   by one side and the world feels alive with both factions present.
 *
 * PERFORMANCE:
 *   - Only MinAmount–MaxAmount bots exist at any time (vs. hundreds globally)
 *   - Only the player's current zone has loaded grids with bots in them
 *   - DespawnWandererBot handles both live bots AND pending spawn queue entries
 *   - Spawn stagger (500ms per bot) is handled by the existing BotDataMgr::Update() loop
 *   - Config reads use sConfigMgr which caches values — no disk I/O per call
 *   - Zone change handler is O(n) in current bot count — fast for 5–15 bots
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
        LOG_INFO("server.loading", "SmartWandering: OnPlayerLogin for {} (GUID {}), zone={}, spare pool={}",
            player->GetName(), player->GetGUID().GetCounter(), zoneId,
            BotDataMgr::GetAvailableWanderingBotCount());
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
        // Keep default consistent with the rest of the module: master switch defaults ON,
        // but each feature defaults OFF unless explicitly enabled in config.
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
        {
            LOG_INFO("server.loading", "SmartWandering: Player {} already has {} bots in zone {}, skipping",
                player->GetName(), state.spawnedEntries.size(), newZone);
            return;
        }

        // Cooldown check — prevent zone-border flapping
        uint32 cooldownSec = sConfigMgr->GetOption<uint32>("CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown", 30);
        uint32 now = static_cast<uint32>(GameTime::GetGameTime().count());
        if (state.lastZoneChange > 0 && (now - state.lastZoneChange) < cooldownSec)
        {
            LOG_INFO("server.loading", "SmartWandering: Player {} zone change on cooldown ({}/{}s)",
                player->GetName(), now - state.lastZoneChange, cooldownSec);
            return;
        }
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

        uint32 totalSpawned = 0;
        std::vector<uint32> allEntries;
        allEntries.reserve(count);

        if (IsBalancedFactionEnabled())
        {
            // Faction-balanced spawning.
            // Important: with low counts (e.g. Min=1/Max=2) strict splitting can otherwise
            // result in *zero* spawns when one faction has no spare bots OR the zone has
            // no valid nodes for that faction. We always fall back to "spawn something".

            TeamId playerTeam = player->GetTeamId();

            uint32 allianceCount = count / 2;
            uint32 hordeCount    = count / 2;

            // Give odd remainder to the player's faction (more intuitive than always Horde)
            if (count % 2)
            {
                if (playerTeam == TEAM_ALLIANCE)
                    ++allianceCount;
                else
                    ++hordeCount;
            }

            auto trySpawn = [newZone](uint32 spawnCount, std::vector<uint32>* out, int32 team) -> uint32
            {
                if (!spawnCount)
                    return 0;
                return BotDataMgr::SpawnWanderingBotsInZone(newZone, spawnCount, out, team);
            };

            std::vector<uint32> allianceEntries;
            std::vector<uint32> hordeEntries;

            uint32 aSpawned = 0;
            uint32 hSpawned = 0;

            // Spawn in an order biased towards the player's faction.
            if (playerTeam == TEAM_ALLIANCE)
            {
                aSpawned = trySpawn(allianceCount, &allianceEntries, ALLIANCE);
                hSpawned = trySpawn(hordeCount, &hordeEntries, HORDE);
            }
            else
            {
                hSpawned = trySpawn(hordeCount, &hordeEntries, HORDE);
                aSpawned = trySpawn(allianceCount, &allianceEntries, ALLIANCE);
            }

            totalSpawned = aSpawned + hSpawned;

            // If we couldn't fulfill the requested total, try to allocate the remainder.
            if (totalSpawned < count)
            {
                uint32 missing = count - totalSpawned;

                // First try the player's faction again (in case the first pass failed due to
                // RNG/level-bracket mismatch on some candidates).
                if (playerTeam == TEAM_ALLIANCE)
                {
                    std::vector<uint32> extra;
                    uint32 extraSpawned = trySpawn(missing, &extra, ALLIANCE);
                    aSpawned += extraSpawned;
                    totalSpawned += extraSpawned;
                    missing -= extraSpawned;
                    allianceEntries.insert(allianceEntries.end(), extra.begin(), extra.end());
                }
                else
                {
                    std::vector<uint32> extra;
                    uint32 extraSpawned = trySpawn(missing, &extra, HORDE);
                    hSpawned += extraSpawned;
                    totalSpawned += extraSpawned;
                    missing -= extraSpawned;
                    hordeEntries.insert(hordeEntries.end(), extra.begin(), extra.end());
                }

                // Then try the opposite faction.
                if (missing)
                {
                    std::vector<uint32> extra;
                    uint32 extraSpawned = (playerTeam == TEAM_ALLIANCE)
                        ? trySpawn(missing, &extra, HORDE)
                        : trySpawn(missing, &extra, ALLIANCE);

                    if (playerTeam == TEAM_ALLIANCE)
                    {
                        hSpawned += extraSpawned;
                        hordeEntries.insert(hordeEntries.end(), extra.begin(), extra.end());
                    }
                    else
                    {
                        aSpawned += extraSpawned;
                        allianceEntries.insert(allianceEntries.end(), extra.begin(), extra.end());
                    }

                    totalSpawned += extraSpawned;
                    missing -= extraSpawned;
                }

                // Final fallback: ignore faction split and spawn whatever the spare pool allows.
                if (missing)
                {
                    std::vector<uint32> extra;
                    uint32 extraSpawned = BotDataMgr::SpawnWanderingBotsInZone(newZone, missing, &extra);
                    totalSpawned += extraSpawned;
                    allEntries.insert(allEntries.end(), extra.begin(), extra.end());
                }
            }

            allEntries.insert(allEntries.end(), allianceEntries.begin(), allianceEntries.end());
            allEntries.insert(allEntries.end(), hordeEntries.begin(), hordeEntries.end());

            BOT_LOG_INFO("module", "SmartWandering: Balanced spawn in zone {} — {} Alliance, {} Horde (player {})",
                newZone, aSpawned, hSpawned, player->GetName());
        }
        else
        {
            // Standard spawning: all factions, natural distribution from spare pool
            totalSpawned = BotDataMgr::SpawnWanderingBotsInZone(newZone, count, &allEntries);
        }

        if (totalSpawned > 0)
        {
            state.spawnedEntries = std::move(allEntries);
            state.currentZoneId = newZone;
            if (!IsBalancedFactionEnabled())
            {
                BOT_LOG_INFO("module", "SmartWandering: Spawned {} bots in zone {} for player {}",
                    totalSpawned, newZone, player->GetName());
            }
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
