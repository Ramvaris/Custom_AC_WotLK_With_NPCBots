/*
 * Custom Player Commands — .special and .mountup
 * Config: CustomRamvaris.Commands.Enable
 *
 * .special — Opens a gossip menu to summon custom NPCs, open bank, open mailbox.
 *            NPCs spawn as temporary summons for 60 seconds.
 * .mountup — Auto-mount based on riding skill and current zone.
 *            If already mounted: dismount and cast 81003 (custom speed buff).
 *            Supports flying (Outland, Northrend with Cold Weather Flying, etc.)
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Player.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ScriptedGossip.h"
#include "Config.h"

using namespace Acore::ChatCommands;

// ============================================================================
// Constants
// ============================================================================

enum CustomCommandConstants
{
    SPECIAL_GOSSIP_SENDER       = 99999,    // Identifies .special gossip selections
    SPECIAL_ACTION_OPEN_BANK    = 100,
    SPECIAL_ACTION_OPEN_MAIL    = 101,

    CUSTOM_SPEED_BUFF_SPELL     = 81003,    // Cast on dismount
    COLD_WEATHER_FLYING_SPELL   = 54197,

    MOUNT_SPELL_310_FLY         = 42668,    // Artisan flying mount
    MOUNT_SPELL_150_FLY         = 42667,    // Expert flying mount
    MOUNT_SPELL_100_GROUND      = 42683,    // Epic ground mount
};

struct SummonableNPC
{
    uint32 entry;
    char const* name;
};

static const SummonableNPC SummonMap[] =
{
    { 290011, "Ling (Reagent Bank)" },
    { 299902, "Ashari (Vendor)" },
    { 299900, "Lilly (Helper)" },
    { 299901, "Lord Squeak (Emblems)" },
    { 300000, "Cromi (Instance Reset)" },
    { 299903, "Ciel (Mounts)" },
    { 200002, "Rename (Pet Renamer)" },
    { 190011, "Warpweaver (Transmog)" },
};

static constexpr uint32 SUMMON_MAP_SIZE = sizeof(SummonMap) / sizeof(SummonMap[0]);

static bool IsCommandsEnabled()
{
    return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
           sConfigMgr->GetOption<bool>("CustomRamvaris.Commands.Enable", false);
}

// ============================================================================
// Dynamic gossip text helper (same technique as npc_lilly_gossip.cpp)
// ============================================================================
static void SendDynamicGossipText(Player* player, std::string const& text, uint32 textId = 0x7FFFFFFF)
{
    WorldPacket data(SMSG_NPC_TEXT_UPDATE, 100);
    data << textId;
    for (uint8 i = 0; i < 8; ++i)
    {
        data << float(0);
        data << text;
        data << text;
        data << uint32(0);
        for (uint8 e = 0; e < 3; ++e)
        {
            data << uint32(0);
            data << uint32(0);
        }
    }
    player->SendDirectMessage(&data);
}

// ============================================================================
// CommandScript — .special and .mountup registration
// ============================================================================
class custom_commandscript : public CommandScript
{
public:
    custom_commandscript() : CommandScript("custom_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "special", HandleSpecialCommand, SEC_PLAYER, Console::No },
            { "mountup", HandleMountupCommand, SEC_PLAYER, Console::No },
        };
        return commandTable;
    }

    // --- .special command ---
    static bool HandleSpecialCommand(ChatHandler* handler)
    {
        if (!IsCommandsEnabled())
        {
            handler->SendSysMessage("This command is not enabled on this server.");
            return true;
        }

        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        ClearGossipMenuFor(player);
        for (uint32 i = 0; i < SUMMON_MAP_SIZE; ++i)
        {
            std::string label = "Summon " + std::string(SummonMap[i].name);
            // action = i+1 (1-based), sender = SPECIAL_GOSSIP_SENDER to identify our gossip
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, label, SPECIAL_GOSSIP_SENDER, i + 1);
        }
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Open Bank", SPECIAL_GOSSIP_SENDER, SPECIAL_ACTION_OPEN_BANK);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Open Mailbox", SPECIAL_GOSSIP_SENDER, SPECIAL_ACTION_OPEN_MAIL);

        SendDynamicGossipText(player, "Select a service or an NPC to summon for 60 seconds:");
        SendGossipMenuFor(player, 0x7FFFFFFF, player->GetGUID());
        return true;
    }

    // --- .mountup command ---
    static bool HandleMountupCommand(ChatHandler* handler)
    {
        if (!IsCommandsEnabled())
        {
            handler->SendSysMessage("This command is not enabled on this server.");
            return true;
        }

        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        // If already mounted, dismount and cast custom speed buff
        if (player->IsMounted())
        {
            player->Dismount();
            player->CastSpell(player, CUSTOM_SPEED_BUFF_SPELL, false);
            return true;
        }

        uint16 ridingLevel = player->GetSkillValue(SKILL_RIDING);
        uint32 currentMapId = player->GetMapId();

        // Maps where flying is normally allowed
        static const std::unordered_set<uint32> flyingMaps =
        {
            0, 1, 530, 571,       // Main continents
            401, 443, 461,        // Misc instances
            482, 512, 540,        // Misc instances
        };

        bool canFly = false;
        if (flyingMaps.count(currentMapId))
        {
            if (currentMapId == 571) // Northrend requires Cold Weather Flying
                canFly = player->HasSpell(COLD_WEATHER_FLYING_SPELL);
            else
                canFly = true;
        }

        if (canFly)
        {
            if (ridingLevel >= 300)
                player->CastSpell(player, MOUNT_SPELL_310_FLY, true);
            else if (ridingLevel >= 225)
                player->CastSpell(player, MOUNT_SPELL_150_FLY, true);
            else
                player->CastSpell(player, MOUNT_SPELL_100_GROUND, true);
        }
        else
        {
            // Ground mount only
            player->CastSpell(player, MOUNT_SPELL_100_GROUND, true);
        }

        return true;
    }
};

// ============================================================================
// PlayerScript — handles .special gossip selections
// ============================================================================
class custom_special_gossip : public PlayerScript
{
public:
    custom_special_gossip() : PlayerScript("custom_special_gossip") { }

    void OnPlayerGossipSelect(Player* player, uint32 /*menu_id*/, uint32 sender, uint32 action) override
    {
        if (!IsCommandsEnabled())
            return;
        if (sender != SPECIAL_GOSSIP_SENDER)
            return;

        CloseGossipMenuFor(player);

        if (action == SPECIAL_ACTION_OPEN_BANK)
        {
            player->GetSession()->SendShowBank(player->GetGUID());
        }
        else if (action == SPECIAL_ACTION_OPEN_MAIL)
        {
            player->GetSession()->SendShowMailBox(player->GetGUID());
        }
        else if (action >= 1 && action <= SUMMON_MAP_SIZE)
        {
            uint32 npcId = SummonMap[action - 1].entry;
            player->SummonCreature(npcId,
                player->GetPositionX(), player->GetPositionY(),
                player->GetPositionZ(), player->GetOrientation(),
                TEMPSUMMON_TIMED_DESPAWN, 60000);
        }
    }
};

void AddSC_custom_commands()
{
    new custom_commandscript();
    new custom_special_gossip();
}
