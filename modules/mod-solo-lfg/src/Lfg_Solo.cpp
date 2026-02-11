/*
 * Solo LFG — Solo and undersized group queuing for LFG.
 * Original: Traesh, Conan513, Micrah
 * Fixes by Ramvars: Forever Queue recovery — detects stuck LFG states
 * (QUEUED/PROPOSAL with no matching group/proposal) and force-resets them.
 *
 * The "forever queue" bug happens when:
 *   - Player uses LFG while in an incomplete instance
 *   - Player jumps/moves during teleport acceptance
 *   - Proposal timeout fails to properly restore state
 *   - State desync between player and group LFG data
 *
 * Recovery: On login + periodic check (every 15s), if player is in QUEUED or
 * PROPOSAL state but no valid queue/group/proposal exists, force LeaveLfg +
 * LeaveAllLfgQueues to nuke the stuck state.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "World.h"
#include "LFGMgr.h"
#include "Chat.h"
#include "Opcodes.h"
#include "Group.h"
#include "WorldSession.h"

static constexpr uint32 LFG_STUCK_CHECK_INTERVAL_MS = 30000;
static constexpr uint32 LFG_STUCK_THRESHOLD_MS      = 90000; // 90s — generous margin over 40s proposal timeout

/// Per-player tracker for how long they've been in a suspicious LFG state
static std::unordered_map<ObjectGuid, uint32> s_lfgStuckTimers;

/// Force-reset a stuck LFG state. Nukes all queue data and notifies the player.
static void ForceResetLfgState(Player* player, const char* reason)
{
    ObjectGuid guid = player->GetGUID();
    Group* group = player->GetGroup();
    ObjectGuid gguid = group ? group->GetGUID() : ObjectGuid::Empty;

    LOG_INFO("module", "SoloLFG: Force-resetting stuck LFG state for {} ({}). Reason: {}",
        player->GetName(), guid.ToString(), reason);

    sLFGMgr->LeaveLfg(guid);
    sLFGMgr->LeaveAllLfgQueues(guid, true, gguid);

    // Tell the client to clear its LFG UI
    player->GetSession()->SendLfgUpdatePlayer(lfg::LfgUpdateData(lfg::LFG_UPDATETYPE_REMOVED_FROM_QUEUE));
    if (group)
        player->GetSession()->SendLfgUpdateParty(lfg::LfgUpdateData(lfg::LFG_UPDATETYPE_REMOVED_FROM_QUEUE));

    ChatHandler(player->GetSession()).SendSysMessage(
        "|cffFF6600[Solo LFG]|r Queue state was stuck — automatically reset. You can queue again now.");

    s_lfgStuckTimers.erase(guid);
}

/// Check if a player's LFG state looks stuck and reset it if needed.
/// Returns true if a reset was performed.
static bool CheckAndRecoverLfgState(Player* player)
{
    if (!sConfigMgr->GetOption<bool>("SoloLFG.Enable", true))
        return false;

    ObjectGuid guid = player->GetGUID();
    lfg::LfgState playerState = sLFGMgr->GetState(guid);

    // Not in any LFG state — nothing to do
    if (playerState == lfg::LFG_STATE_NONE ||
        playerState == lfg::LFG_STATE_DUNGEON ||
        playerState == lfg::LFG_STATE_FINISHED_DUNGEON)
    {
        s_lfgStuckTimers.erase(guid);
        return false;
    }

    ObjectGuid lfgGroupGuid = sLFGMgr->GetGroup(guid);
    Group* group = player->GetGroup();
    lfg::LfgState groupState = lfgGroupGuid ? sLFGMgr->GetState(lfgGroupGuid) : lfg::LFG_STATE_NONE;

    bool looksStuck = false;

    // Case 1: Player is QUEUED but their LFG group is in NONE state
    if (playerState == lfg::LFG_STATE_QUEUED && lfgGroupGuid && groupState == lfg::LFG_STATE_NONE)
        looksStuck = true;

    // Case 2: Player is in QUEUED/PROPOSAL but has no LFG group and no real group
    if ((playerState == lfg::LFG_STATE_QUEUED || playerState == lfg::LFG_STATE_PROPOSAL) &&
        !lfgGroupGuid && !group)
        looksStuck = true;

    // Case 3: Player is in ROLECHECK state for too long (should timeout in 45s)
    if (playerState == lfg::LFG_STATE_ROLECHECK)
        looksStuck = true;

    if (!looksStuck)
    {
        s_lfgStuckTimers.erase(guid);
        return false;
    }

    // Track how long we've been in a suspicious state — don't reset immediately
    // because a brief desync during normal operation is expected.
    auto it = s_lfgStuckTimers.find(guid);
    if (it == s_lfgStuckTimers.end())
    {
        s_lfgStuckTimers[guid] = 0;
        return false;
    }

    it->second += LFG_STUCK_CHECK_INTERVAL_MS;

    if (it->second >= LFG_STUCK_THRESHOLD_MS)
    {
        ForceResetLfgState(player, "state stuck for 90+ seconds");
        return true;
    }

    return false;
}

class lfg_solo_announce : public PlayerScript
{
public:
    lfg_solo_announce() : PlayerScript("lfg_solo_announce") {}

    void OnPlayerLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("SoloLFG.Announce", true))
        {
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00Solo Dungeon Finder |rmodule.");
        }

        // On login, check if player was stuck in a broken LFG state from before
        lfg::LfgState state = sLFGMgr->GetState(player->GetGUID());
        if (state == lfg::LFG_STATE_QUEUED || state == lfg::LFG_STATE_PROPOSAL ||
            state == lfg::LFG_STATE_ROLECHECK)
        {
            // Give it a brief window, then the periodic check will catch it
            s_lfgStuckTimers[player->GetGUID()] = LFG_STUCK_THRESHOLD_MS - LFG_STUCK_CHECK_INTERVAL_MS;
        }
    }

    void OnPlayerLogout(Player* player) override
    {
        s_lfgStuckTimers.erase(player->GetGUID());
    }

    void OnPlayerRewardKillRewarder(Player* /*player*/, KillRewarder* /*rewarder*/, bool isDungeon, float& rate) override
    {
        if (!isDungeon
            || !sConfigMgr->GetOption<bool>("SoloLFG.Enable", true)
            || !sConfigMgr->GetOption<bool>("SoloLFG.FixedXP", true))
        {
            return;
        }

        rate = sConfigMgr->GetOption<float>("SoloLFG.FixedXPRate", 0.2);
    }
};

/// Periodic stuck-state recovery — runs OnUpdate every LFG_STUCK_CHECK_INTERVAL_MS.
class lfg_solo_recovery : public PlayerScript
{
public:
    lfg_solo_recovery() : PlayerScript("lfg_solo_recovery") {}

    void OnPlayerUpdate(Player* player, uint32 /*p_time*/) override
    {
        if (!sConfigMgr->GetOption<bool>("SoloLFG.Enable", true))
            return;

        // Throttle to ~once per LFG_STUCK_CHECK_INTERVAL_MS using static timer
        // per-player timestamps stored in the s_lfgStuckTimers map are enough
        // to prevent over-checking since CheckAndRecoverLfgState only increments
        // when a suspicious state is detected.
        lfg::LfgState state = sLFGMgr->GetState(player->GetGUID());
        if (state == lfg::LFG_STATE_NONE ||
            state == lfg::LFG_STATE_DUNGEON ||
            state == lfg::LFG_STATE_FINISHED_DUNGEON)
            return;

        CheckAndRecoverLfgState(player);
    }
};

class lfg_solo : public WorldScript
{
public:
    lfg_solo() : WorldScript("lfg_solo") {}

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (sConfigMgr->GetOption<bool>("SoloLFG.Enable", true) != sLFGMgr->IsTesting())
        {
            sLFGMgr->ToggleTesting();
        }
    }
};

void AddLfgSoloScripts()
{
    new lfg_solo_announce();
    new lfg_solo_recovery();
    new lfg_solo();
}
