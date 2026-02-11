/*
 * Custom Player Commands — .special, .guardianscale, .petscale
 * Config: CustomRamvaris.SpecialCommand.Enable,
 *         CustomRamvaris.GuardianScale.Enable, CustomRamvaris.PetScale.Enable
 *
 * .special        — Opens a gossip menu to summon custom NPCs, open bank, open mailbox.
 *                   NPCs spawn as temporary summons for 60 seconds.
 * .guardianscale  — Doubles Soul Keeper guardian visual scale (caps at 5x base).
 * .petscale       — Doubles any pet visual scale (Hunter/Warlock/DK, caps at 5x base).
 *
 * (.mountup removed — lottery enchant speed/fly system replaces manual mounting.)
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ScriptedGossip.h"
#include "Config.h"
#include "npc_summon_flavor.h"

using namespace Acore::ChatCommands;

// ============================================================================
// Constants
// ============================================================================

enum CustomCommandConstants
{
    SPECIAL_GOSSIP_SENDER       = 99999,    // Identifies .special gossip selections
    SPECIAL_ACTION_OPEN_BANK    = 100,
    SPECIAL_ACTION_OPEN_MAIL    = 101,

    MAX_VISUAL_SCALE            = 5,        // Cap for .guardianscale / .petscale
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
    { 299903, "Ciel (Mounts)" },
    { 200002, "Rename (Pet Renamer)" },
    { 190011, "Warpweaver (Transmog)" },
};

static constexpr uint32 SUMMON_MAP_SIZE = sizeof(SummonMap) / sizeof(SummonMap[0]);

static bool IsSpecialEnabled()
{
    return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
           sConfigMgr->GetOption<bool>("CustomRamvaris.SpecialCommand.Enable", false);
}

static bool IsGuardianScaleEnabled()
{
    return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
           sConfigMgr->GetOption<bool>("CustomRamvaris.GuardianScale.Enable", false);
}

static bool IsPetScaleEnabled()
{
    return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
           sConfigMgr->GetOption<bool>("CustomRamvaris.PetScale.Enable", false);
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
// CommandScript — .special, .mountup, .guardianscale, .petscale registration
// ============================================================================
class custom_commandscript : public CommandScript
{
public:
    custom_commandscript() : CommandScript("custom_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "special",       HandleSpecialCommand,       SEC_PLAYER, Console::No },
            { "guardianscale", HandleGuardianScaleCommand, SEC_PLAYER, Console::No },
            { "petscale",      HandlePetScaleCommand,      SEC_PLAYER, Console::No },
            { "ramhelp",       HandleRamhelpCommand,       SEC_PLAYER, Console::No },
        };
        return commandTable;
    }

    // --- .special command ---
    static bool HandleSpecialCommand(ChatHandler* handler)
    {
        if (!IsSpecialEnabled())
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

    // --- .ramhelp command ---
    // Lists ALL custom slash commands from every custom module on the server.
    // Always available — no config gate. Every player can see the help.
    static bool HandleRamhelpCommand(ChatHandler* handler)
    {
        handler->SendSysMessage("|cff00CCFF=== Ramvaris Custom Commands ===|r");

        // mod-custom-ramvaris
        handler->SendSysMessage("|cffFFD700[mod-custom-ramvaris]|r");
        handler->SendSysMessage("  .special        — Summon custom NPCs / Open bank / Open mailbox");
        handler->SendSysMessage("  .enchants       — View lottery enchants on all your items (gossip)");
        handler->SendSysMessage("  .reroll         — Reroll lottery enchants on a bag item (500g)");
        handler->SendSysMessage("  .flylock        — Toggle flight on/off (if you have a fly enchant)");
        handler->SendSysMessage("  .guardianscale  — Double your Soul Keeper guardian's visual size");
        handler->SendSysMessage("  .petscale       — Double your pet's visual size (Hunter/Lock/DK)");
        handler->SendSysMessage("  .ramhelp        — This help page");

        // mod-soul-keeper
        handler->SendSysMessage("|cffFFD700[mod-soul-keeper]|r");
        handler->SendSysMessage("  .soul absorb    — Capture a creature into your soul collection");
        handler->SendSysMessage("  .soul summon    — Open soul list / summon guardian by index");
        handler->SendSysMessage("  .soul dismiss   — Dismiss your active Soul Keeper guardian");
        handler->SendSysMessage("  .soul return    — Recall guardian back (60s cooldown)");
        handler->SendSysMessage("  .soul rename    — Rename your summoned guardian");
        handler->SendSysMessage("  .soul search    — Search captured souls by name");

        // mod-guildhouse
        handler->SendSysMessage("|cffFFD700[mod-guildhouse]|r");
        handler->SendSysMessage("  .gh teleport    — Teleport to your guild house");
        handler->SendSysMessage("  .gh butler      — Spawn the Guild House Butler (GM only)");

        // mod-transmog
        handler->SendSysMessage("|cffFFD700[mod-transmog]|r");
        handler->SendSysMessage("  .transmog       — Toggle transmog visual visibility");
        handler->SendSysMessage("  .transmog portable — Summon a portable transmogrifier NPC");

        handler->SendSysMessage("|cff00CCFF================================|r");
        return true;
    }

    // --- .guardianscale command ---
    // Doubles the visual scale of the player's active Soul Keeper guardian (caps at 5x base).
    // Only works on Soul Keeper guardians (IsSoulKeeperGuardian check).
    static bool HandleGuardianScaleCommand(ChatHandler* handler)
    {
        if (!IsGuardianScaleEnabled())
        {
            handler->SendSysMessage("This command is not enabled on this server.");
            return true;
        }

        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        // Find the player's Soul Keeper guardian in their controlled units
        Unit* skGuardian = nullptr;
        for (Unit* controlled : player->m_Controlled)
        {
            if (controlled && controlled->IsAlive() && controlled->IsSoulKeeperGuardian())
            {
                skGuardian = controlled;
                break;
            }
        }

        if (!skGuardian)
        {
            handler->SendSysMessage("You don't have an active Soul Keeper guardian.");
            return true;
        }

        float currentScale = skGuardian->GetObjectScale();
        float newScale = currentScale * 2.0f;
        if (newScale > static_cast<float>(MAX_VISUAL_SCALE))
            newScale = static_cast<float>(MAX_VISUAL_SCALE);

        if (currentScale >= static_cast<float>(MAX_VISUAL_SCALE))
        {
            handler->PSendSysMessage("Guardian is already at maximum scale ({:.0f}x).", currentScale);
            return true;
        }

        skGuardian->SetObjectScale(newScale);
        handler->PSendSysMessage("Guardian scale: {:.1f}x -> {:.1f}x", currentScale, newScale);
        return true;
    }

    // --- .petscale command ---
    // Doubles the visual scale of any active pet (Hunter/Warlock/DK).
    // Uses IsPet() check — works for all proper pet types, NOT guardians.
    static bool HandlePetScaleCommand(ChatHandler* handler)
    {
        if (!IsPetScaleEnabled())
        {
            handler->SendSysMessage("This command is not enabled on this server.");
            return true;
        }

        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        Pet* pet = player->GetPet();
        if (!pet)
        {
            handler->SendSysMessage("You don't have an active pet.");
            return true;
        }

        float currentScale = pet->GetObjectScale();
        float newScale = currentScale * 2.0f;
        if (newScale > static_cast<float>(MAX_VISUAL_SCALE))
            newScale = static_cast<float>(MAX_VISUAL_SCALE);

        if (currentScale >= static_cast<float>(MAX_VISUAL_SCALE))
        {
            handler->PSendSysMessage("Pet is already at maximum scale ({:.0f}x).", currentScale);
            return true;
        }

        pet->SetObjectScale(newScale);
        handler->PSendSysMessage("Pet scale: {:.1f}x -> {:.1f}x", currentScale, newScale);
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
        if (!IsSpecialEnabled())
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
            if (Creature* summon = player->SummonCreature(npcId,
                player->GetPositionX(), player->GetPositionY(),
                player->GetPositionZ(), player->GetOrientation(),
                TEMPSUMMON_TIMED_DESPAWN, 60000))
            {
                // Set owner GUID so NPC scripts with owner-based CanBeSeen work
                // (e.g., npc_transmogrifier portable NPC visibility gate).
                // TEMPSUMMON_TIMED_DESPAWN doesn't set UNIT_FIELD_SUMMONEDBY —
                // without this, GetOwner() returns nullptr and the NPC is invisible.
                summon->SetOwnerGUID(player->GetGUID());

                DoSummonFlavorText(summon, player);
            }
        }
    }
};

void AddSC_custom_commands()
{
    new custom_commandscript();
    new custom_special_gossip();
}
