/*
 * Lilly Helper NPC (Entry 299900) — Gossip + Summon Flavor
 * Config: CustomRamvaris.LillyGossip.Enable
 *
 * Services: Teleportation (cities, dungeons, points of interest),
 *           Instance Reset, Exalted Talent Point Purchase.
 * Also handles Lilly's summon flavor text via IsSummonedBy.
 *
 * Requires creature_template entry 299900 with ScriptName = "npc_custom_lilly"
 * and npc_flags including UNIT_NPC_FLAG_GOSSIP (1).
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "InstanceSaveMgr.h"
#include "Config.h"

// Custom gossip text injection — same technique as the Lua GossipSetText
// Sends a fake SMSG_NPC_TEXT_UPDATE with a dynamic text string the client caches
static void SendDynamicGossipText(Player* player, std::string const& text, uint32 textId = 0x7FFFFFFF)
{
    WorldPacket data(SMSG_NPC_TEXT_UPDATE, 100);
    data << textId;
    for (uint8 i = 0; i < 8; ++i) // MAX_GOSSIP_TEXT_OPTIONS = 8
    {
        data << float(0);       // Probability
        data << text;           // Text_0
        data << text;           // Text_1
        data << uint32(0);      // Language
        for (uint8 e = 0; e < 3; ++e) // MAX_GOSSIP_TEXT_EMOTES = 3
        {
            data << uint32(0);  // EmoteDelay
            data << uint32(0);  // EmoteId
        }
    }
    player->SendDirectMessage(&data);
}

// Teleport helper — deducts copper, whispers, teleports
static void TeleportService(Player* player, Creature* creature, float x, float y, float z, float o, uint32 mapId, uint32 costCopper)
{
    CloseGossipMenuFor(player);
    player->ModifyMoney(-static_cast<int32>(costCopper));
    creature->Whisper("Safe travels, " + std::string(player->GetName()) + "!", LANG_UNIVERSAL, player);
    player->TeleportTo(mapId, x, y, z, o);
}

// Custom faction ID for reputation-gated services
enum LillyConstants
{
    LILLY_NPC_ENTRY         = 299900,
    LILLY_GOSSIP_TEXT_ID    = 0x7FFFFFFF,
    EMBLEM_ITEM_ID          = 60001,
    EMBLEM_COST             = 7,
    CITY_TELEPORT_COST      = 1000,       // 10 silver
    DUNGEON_TELEPORT_COST   = 200000,     // 20 gold
    POI_TELEPORT_COST       = 200000,     // 20 gold
    ENEMY_TERRITORY_COST    = 1000000,    // 100 gold
    INSTANCE_RESET_COST     = 50000,      // 5 gold
    CUSTOM_FACTION_ID       = 1161,
};

static bool IsLillyEnabled()
{
    return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
           sConfigMgr->GetOption<bool>("CustomRamvaris.LillyGossip.Enable", false);
}

// ============================================================================
// Lilly CreatureScript — Gossip + AI
// ============================================================================
class npc_custom_lilly : public CreatureScript
{
public:
    npc_custom_lilly() : CreatureScript("npc_custom_lilly") { }

    // --- Summon Flavor Text (from else.lua OnLillySummon) ---
    struct npc_custom_lilly_ai : public ScriptedAI
    {
        npc_custom_lilly_ai(Creature* creature) : ScriptedAI(creature) { }

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (!IsLillyEnabled())
                return;

            std::string name = summoner ? summoner->GetName() : "adventurer";
            uint32 choice = urand(1, 12);
            switch (choice)
            {
                case 1:
                    me->Say("Greetings, " + name + ". How may I serve the will of Ramires today?", LANG_UNIVERSAL);
                    me->TextEmote("Lilly bows gracefully.");
                    break;
                case 2:  me->Say("I live to serve... and to reset your hearthstone cooldown.", LANG_UNIVERSAL); break;
                case 3:
                    me->Say("Do you require assistance? or just company?", LANG_UNIVERSAL);
                    me->TextEmote("Lilly smiles warmly.");
                    break;
                case 4:
                    me->Say("The great Ramires told me you might need help. He didn't mention you'd need *this* much help.", LANG_UNIVERSAL);
                    me->TextEmote("Lilly winks.");
                    break;
                case 5:  me->Say("Teleportation, banking, advice... I do it all. Except laundry.", LANG_UNIVERSAL); break;
                case 6:  me->Say("I was just organizing the master's library. What do you need?", LANG_UNIVERSAL); break;
                case 7:  me->Say("You rang? Oh wait, that's a different franchise.", LANG_UNIVERSAL); break;
                case 8:  me->Say("I am ready to assist. Please don't ask me to tank.", LANG_UNIVERSAL); break;
                case 9:  me->Say("Your wish is my command. Within the boundaries of the script execution time limit.", LANG_UNIVERSAL); break;
                case 10:
                    me->Say("A helper's work is never done.", LANG_UNIVERSAL);
                    me->TextEmote("Lilly dusts off her shoulder.");
                    break;
                case 11: me->Say("How can I make your Azerothian life easier today?", LANG_UNIVERSAL); break;
                default: me->Say("At your service! No, I don't know where Mankrik's wife is.", LANG_UNIVERSAL); break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_custom_lilly_ai(creature);
    }

    // --- Gossip Hello ---
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!IsLillyEnabled())
            return false; // Fall through to default gossip

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Let me see your services", 0, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Nevermind", 0, 999);
        SendDynamicGossipText(player, "Greetings, $n.$B$BThe great Ramires gave me permission to help you out on your quest.$B$BHow may I help you today?");
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
        return true;
    }

    // --- Gossip Select ---
    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!IsLillyEnabled())
            return false;

        ClearGossipMenuFor(player);

        switch (action)
        {
            case 1: // Main services menu
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to use the teleportation service", 0, 3);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to use the instance reset service", 0, 4);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to use the exalted special service", 0, 2);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I have no need for your services right now", 0, 999);
                SendDynamicGossipText(player, "I am allowed to offer you the following services.");
                SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                break;

            case 2: // Exalted talent point purchase
                HandleExaltedService(player, creature);
                break;

            case 3: // Teleportation categories
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to travel to a bigger city...", 0, 6);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to travel to a dungeon...", 0, 8);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to travel to a point of interest...", 0, 7);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to use a different service", 0, 1);
                SendDynamicGossipText(player, "So, let's choose your destination category first!");
                SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                break;

            case 4: // Instance reset confirmation
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Fine with me...", 0, 5);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Nevermind...", 0, 1);
                SendDynamicGossipText(player, "So you want to reset your instances?$B$BThen let's see... how about 5 gold coins?");
                SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                break;

            case 5: // Instance reset execution
                HandleInstanceReset(player, creature);
                break;

            case 6: // City teleports
                HandleCityTeleports(player, creature);
                break;

            case 7: // Point of interest teleports
                HandlePOITeleports(player, creature);
                break;

            case 8: // Dungeon teleports
                HandleDungeonTeleports(player, creature);
                break;

            // === City teleport destinations ===
            case 10: TeleportService(player, creature, -8833.38f, 628.628f, 94.0066f, 1.06535f, 0, CITY_TELEPORT_COST); break;     // Stormwind
            case 11: TeleportService(player, creature, -4918.88f, -940.406f, 501.564f, 5.42347f, 0, CITY_TELEPORT_COST); break;    // Ironforge
            case 12: TeleportService(player, creature, 9949.56f, 2284.21f, 1341.4f, 1.59587f, 1, CITY_TELEPORT_COST); break;       // Darnassus
            case 13: TeleportService(player, creature, -3965.7f, -11653.6f, -138.844f, 0.852154f, 530, CITY_TELEPORT_COST); break;  // Exodar
            case 14: // Shattrath (level 60+)
                if (player->GetLevel() < 60)
                {
                    SendDynamicGossipText(player, "Sorry, but you need to be level 60 to travel to Shattrath.");
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "...", 0, 6);
                    SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                }
                else
                    TeleportService(player, creature, -1838.16f, 5301.79f, -12.428f, 5.9517f, 530, CITY_TELEPORT_COST);
                break;
            case 15: // Dalaran (level 70+)
                if (player->GetLevel() < 70)
                {
                    SendDynamicGossipText(player, "Sorry, but you need to be level 70 to travel to Dalaran.");
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "...", 0, 6);
                    SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                }
                else
                    TeleportService(player, creature, 5804.15f, 624.771f, 647.767f, 1.64f, 571, CITY_TELEPORT_COST);
                break;
            case 16: TeleportService(player, creature, 1629.85f, -4373.64f, 31.5573f, 3.69762f, 1, CITY_TELEPORT_COST); break;     // Orgrimmar
            case 17: TeleportService(player, creature, -1277.37f, 124.804f, 131.287f, 5.22274f, 1, CITY_TELEPORT_COST); break;      // Thunderbluff
            case 18: TeleportService(player, creature, 1584.14f, 240.308f, -52.1534f, 0.041793f, 0, CITY_TELEPORT_COST); break;     // Undercity
            case 19: TeleportService(player, creature, 9487.69f, -7279.2f, 14.2866f, 6.16478f, 530, CITY_TELEPORT_COST); break;     // Silvermoon

            // === Dungeon teleport destinations ===
            case 21: // Ragefire Chasm (Alliance — expensive, enemy territory)
                if (player->GetMoney() < ENEMY_TERRITORY_COST)
                {
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Fine... another time then", 0, 8);
                    SendDynamicGossipText(player, "Sorry, you do not have enough coinage to travel to the Ragefire Chasm.");
                    SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                }
                else
                    TeleportService(player, creature, 1811.78f, -4410.5f, -18.4704f, 5.20165f, 1, ENEMY_TERRITORY_COST);
                break;
            case 22: TeleportService(player, creature, 1811.78f, -4410.5f, -18.4704f, 5.20165f, 1, DUNGEON_TELEPORT_COST); break;  // Ragefire (Horde)
            case 23: TeleportService(player, creature, -805.049f, -2032.03f, 95.8796f, 6.18912f, 1, DUNGEON_TELEPORT_COST); break;  // Wailing Caverns
            case 24: TeleportService(player, creature, -8779.9f, 834.349f, 94.6801f, 0.653013f, 0, DUNGEON_TELEPORT_COST); break;   // Stockades (Alliance)
            case 25: // Stockades (Horde — expensive, enemy territory)
                if (player->GetMoney() < ENEMY_TERRITORY_COST)
                {
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Fine... another time then", 0, 8);
                    SendDynamicGossipText(player, "Sorry, you do not have enough coinage to travel to the Stockades.");
                    SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                }
                else
                    TeleportService(player, creature, -8779.9f, 834.349f, 94.6801f, 0.653013f, 0, ENEMY_TERRITORY_COST);
                break;
            case 26: TeleportService(player, creature, -4470.28f, -1677.77f, 81.3925f, 1.16302f, 1, DUNGEON_TELEPORT_COST); break;  // Razorfen Kraul
            case 27: TeleportService(player, creature, -11208.7f, 1673.52f, 24.6361f, 1.51067f, 0, DUNGEON_TELEPORT_COST); break;   // Deadmines
            case 28: TeleportService(player, creature, 4249.99f, 740.102f, -25.671f, 1.34062f, 1, DUNGEON_TELEPORT_COST); break;    // Blackfathom Deeps
            case 29: TeleportService(player, creature, -234.675f, 1561.63f, 76.8921f, 1.24031f, 0, DUNGEON_TELEPORT_COST); break;   // Shadowfang Keep
            case 30: TeleportService(player, creature, 2872.6f, -764.398f, 160.332f, 5.05735f, 0, DUNGEON_TELEPORT_COST); break;    // Scarlet Monastery
            case 31: TeleportService(player, creature, -5163.54f, 925.423f, 257.181f, 1.57423f, 0, DUNGEON_TELEPORT_COST); break;   // Gnomeregan
            case 32: TeleportService(player, creature, -4657.3f, -2519.35f, 81.0529f, 4.54808f, 1, DUNGEON_TELEPORT_COST); break;   // Razorfen Downs
            case 33: TeleportService(player, creature, -6801.19f, -2893.02f, 9.00388f, 0.158639f, 1, DUNGEON_TELEPORT_COST); break; // Zul'Farrak
            case 34: TeleportService(player, creature, -6071.37f, -2955.16f, 209.782f, 0.015708f, 0, DUNGEON_TELEPORT_COST); break; // Uldaman
            case 35: TeleportService(player, creature, -7179.34f, -921.212f, 165.821f, 5.09599f, 0, DUNGEON_TELEPORT_COST); break;  // BRD
            case 36: TeleportService(player, creature, -10177.9f, -3994.9f, -111.239f, 6.01885f, 0, DUNGEON_TELEPORT_COST); break;  // Sunken Temple
            case 37: TeleportService(player, creature, -7527.05f, -1226.77f, 285.732f, 5.29626f, 0, DUNGEON_TELEPORT_COST); break;  // LBRS/UBRS
            case 38: TeleportService(player, creature, 1269.64f, -2556.21f, 93.6088f, 0.620623f, 0, DUNGEON_TELEPORT_COST); break;  // Scholomance
            case 39: TeleportService(player, creature, 3352.92f, -3379.03f, 144.782f, 6.25978f, 0, DUNGEON_TELEPORT_COST); break;   // Stratholme
            case 40: TeleportService(player, creature, -3980.8f, 789.005f, 161.007f, 4.71945f, 1, DUNGEON_TELEPORT_COST); break;    // Dire Maul East

            // === Points of Interest ===
            case 41: TeleportService(player, creature, -4819.56f, -974.088f, 464.709f, 3.963f, 0, POI_TELEPORT_COST); break;       // Old Ironforge
            case 42: TeleportService(player, creature, 7367.77f, -1560.74f, 163.446f, 2.55011f, 1, POI_TELEPORT_COST); break;      // Shatterspear Village
            case 43: TeleportService(player, creature, -4031.92f, -1411.13f, 156.758f, 0.429314f, 0, POI_TELEPORT_COST); break;    // Wetlands Dwarven Village
            case 44: TeleportService(player, creature, -11069.7f, -1795.77f, 53.7318f, 3.09641f, 0, POI_TELEPORT_COST); break;     // Karazhan Crypts
            case 45: TeleportService(player, creature, 5475.99f, -3728.73f, 1593.44f, 5.80284f, 1, POI_TELEPORT_COST); break;      // Hyjal Construction
            case 46: TeleportService(player, creature, -10750.5f, 2432.04f, 6.14444f, 0.240715f, 1, POI_TELEPORT_COST); break;     // Silithus Farm
            case 47: TeleportService(player, creature, 4296.04f, -2763.72f, 16.268f, 0.51962f, 0, POI_TELEPORT_COST); break;       // Quel'thalas Tower
            case 48: TeleportService(player, creature, -1849.54f, -4149.05f, 9.8162f, 3.07668f, 0, POI_TELEPORT_COST); break;      // Arathi Dwarven Farm
            case 49: TeleportService(player, creature, -3857.0f, -3485.0f, 579.64f, 3.3092f, 0, POI_TELEPORT_COST); break;         // Wetlands Hidden Spot
            case 50: TeleportService(player, creature, 3176.63f, -4039.28f, 105.464f, 3.3092f, 329, POI_TELEPORT_COST); break;     // Stratholme Instance
            case 51: TeleportService(player, creature, -5313.68f, -2512.7f, 484.236f, 3.57754f, 0, POI_TELEPORT_COST); break;      // Ortells Hideout
            case 52: TeleportService(player, creature, -6376.91f, 1262.61f, 7.18831f, 2.23557f, 0, POI_TELEPORT_COST); break;      // Newmans Landing
            case 53: TeleportService(player, creature, -98.3923f, 1216.83f, -122.163f, 1.48305f, 369, POI_TELEPORT_COST); break;   // Nessy at Deeprun Tram
            case 54: TeleportService(player, creature, 74.5078f, 1184.51f, -119.551f, 2.90473f, 369, POI_TELEPORT_COST); break;    // Deeprun Other Side
            case 55: TeleportService(player, creature, 2738.87f, -3320.93f, 101.917f, 0.366472f, 169, POI_TELEPORT_COST); break;   // Emerald Dream Forest
            case 56: TeleportService(player, creature, 79.0f, -1.0f, 18.6778f, 0.0f, 44, POI_TELEPORT_COST); break;               // Old Scarlet Monastery

            case 57: // Purchase talent point
                HandleTalentPurchase(player, creature);
                break;

            case 997: // Re-enter the world (force logout)
                CloseGossipMenuFor(player);
                player->GetSession()->LogoutPlayer(true);
                break;

            case 998: // Service unavailable
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "What an odd thing to say", 0, 1);
                SendDynamicGossipText(player, "Sorry, $n.$B$BI think I can't offer you this service right now, even though I proposed it to you.$B$BMaybe you could try again later?");
                SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
                break;

            case 999: // Goodbye
                HandleGoodbye(player, creature);
                break;

            default:
                CloseGossipMenuFor(player);
                break;
        }
        return true;
    }

private:
    // --- Exalted talent point service ---
    static void HandleExaltedService(Player* player, Creature* creature)
    {
        if (player->GetReputationRank(CUSTOM_FACTION_ID) < REP_EXALTED)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Ok. Maybe another time...", 0, 1);
            SendDynamicGossipText(player, "Your reputation with our faction is not nearly high enough for this service...");
        }
        else if (player->HasItemCount(EMBLEM_ITEM_ID, EMBLEM_COST))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Great. I want to purchase it!", 0, 57);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I think I will pass this time", 0, 1);
            SendDynamicGossipText(player, "You have enough reputation with our faction and you have the required 7 Emblems of Ramires.$B$BDo you want to purchase a permanent additional talent point for the Emblems?");
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Ok. I will try to gather them...", 0, 1);
            SendDynamicGossipText(player, "You do not have enough Emblems of Ramires to use this service.$B$BYou need at least 7!");
        }
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
    }

    // --- Instance reset ---
    static void HandleInstanceReset(Player* player, Creature* creature)
    {
        if (player->GetMoney() < INSTANCE_RESET_COST)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Ok...", 0, 1);
            SendDynamicGossipText(player, "You do not have enough money to pay for my services.$B$BSo I guess you have to wait or you might come again with a bigger purse.");
        }
        else
        {
            player->ModifyMoney(-static_cast<int32>(INSTANCE_RESET_COST));
            // Unbind all instances (same pattern as mod-instance-reset and ALE)
            for (uint8 d = 0; d < MAX_DIFFICULTY; ++d)
            {
                BoundInstancesMap const& binds = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), Difficulty(d));
                for (auto itr = binds.begin(); itr != binds.end();)
                {
                    if (itr->first != player->GetMapId())
                    {
                        sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), itr->first, Difficulty(d), true, player);
                        itr = binds.begin(); // Container modified, restart iteration
                    }
                    else
                        ++itr;
                }
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "That's great!", 0, 1);
            SendDynamicGossipText(player, "Thank you!$B$BAll of your instances have been reset.");
        }
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
    }

    // --- City teleport menu ---
    static void HandleCityTeleports(Player* player, Creature* creature)
    {
        bool enoughMoney = player->GetMoney() >= CITY_TELEPORT_COST;

        if (!enoughMoney)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Oh. I will try to get more coinage then.", 0, 3);
            SendDynamicGossipText(player, "You don't seem to be wealthy enough to use my teleportation services for bigger cities.$B$BYou need at least 10 silver coins in your purse!");
        }
        else if (player->GetTeamId() == TEAM_ALLIANCE)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Stormwind", 0, 10);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Ironforge", 0, 11);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Darnassus", 0, 12);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Exodar", 0, 13);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Shattrath", 0, 14);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Dalaran", 0, 15);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Sorry, but I changed my opinion.", 0, 3);
            SendDynamicGossipText(player, "A bigger city then!$B$BBut which one exactly?$B$BAnd remember that I will charge 10 silver coins for each travel!");
        }
        else // Horde
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Orgrimmar", 0, 16);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Thunderbluff", 0, 17);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Undercity", 0, 18);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Silvermoon", 0, 19);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Shattrath", 0, 14);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Dalaran", 0, 15);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Sorry, but I changed my opinion.", 0, 3);
            SendDynamicGossipText(player, "A bigger city then!$B$BBut which one exactly?$B$BAnd remember that I will charge 10 silver coins for each travel!");
        }
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
    }

    // --- Point of interest teleport menu ---
    static void HandlePOITeleports(Player* player, Creature* creature)
    {
        if (player->GetMoney() < POI_TELEPORT_COST)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Oh. I will try to get more coinage then.", 0, 3);
            SendDynamicGossipText(player, "You don't seem to be wealthy enough to use my teleportation services for a point of interest.$B$BYou need at least 20 gold coins in your purse!");
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Old Ironforge", 0, 41);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Shatterspear Village", 0, 42);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Wetlands Dwarven Village", 0, 43);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Karazhan Crypts", 0, 44);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Hyjal Blizzard Construction Site", 0, 45);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Silithus Southern Farm", 0, 46);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Quel'thalas Tower", 0, 47);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Arathi Dwarven Farm", 0, 48);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Wetlands Hidden Spot", 0, 49);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Outsides of Stratholme (Instanced)", 0, 50);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Ortells Hideout in Dun Morogh", 0, 51);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Newmans Landing", 0, 52);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to Nessy at the Deeprun Tram", 0, 53);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the other side of the Deeprun Tram", 0, 54);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Emerald Dream Forest", 0, 55);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Old Scarlet Monastery", 0, 56);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I changed my opinion", 0, 3);
            SendDynamicGossipText(player, "A point of interest then!$B$BBut which one exactly?$B$BRemember that I will charge 20 gold coins for each travel and that I do not guarantee that you can return without your hearthstone!!");
        }
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
    }

    // --- Dungeon teleport menu ---
    static void HandleDungeonTeleports(Player* player, Creature* creature)
    {
        if (player->GetMoney() < DUNGEON_TELEPORT_COST)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Oh. I will try to get more coinage then.", 0, 3);
            SendDynamicGossipText(player, "You don't seem to be wealthy enough to use my teleportation services for a dungeon.$B$BYou need at least 20 gold coins in your purse!");
        }
        else
        {
            bool isAlliance = (player->GetTeamId() == TEAM_ALLIANCE);

            if (isAlliance)
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Bring me to the Ragefire Chasm [Dangerous]", 0, 21);
            else
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Ragefire Chasm", 0, 22);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Wailing Caverns", 0, 23);

            if (isAlliance)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Stockades", 0, 24);
            else
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Bring me to the Stockades [Dangerous]", 0, 25);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Deadmines", 0, 27);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Blackfathom Deeps", 0, 28);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Shadowfang Keep", 0, 29);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Razorfen Kraul", 0, 26);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Scarlet Monastery", 0, 30);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Gnomeregan", 0, 31);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Razorfen Downs", 0, 32);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Zul'Farrak", 0, 33);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Uldaman", 0, 34);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Blackrock Depths", 0, 35);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Sunken Temple", 0, 36);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Blackrock Spire", 0, 37);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Scholomance", 0, 38);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Stratholme", 0, 39);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Bring me to the Dire Maul East", 0, 40);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I changed my opinion", 0, 3);
            SendDynamicGossipText(player, "A dungeon then!$B$BBut which one exactly?$B$BRemember that I will charge 20 gold coins for each travel and that I will only send you to classic dungeons. You can fly in the Outlands and Northrend!$B$BOh. Before I forget it. Travels to enemy territory cost 100 gold coins.");
        }
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
    }

    // --- Talent point purchase ---
    static void HandleTalentPurchase(Player* player, Creature* creature)
    {
        player->DestroyItemCount(EMBLEM_ITEM_ID, EMBLEM_COST, true);
        uint32 freeTalentPoints = player->GetFreeTalentPoints();
        player->SetFreeTalentPoints(freeTalentPoints + 1);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Thank you and yes I still need something", 0, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Thank you and can you help me to re-enter the world?", 0, 997);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Thank you and no that was all", 0, 999);
        SendDynamicGossipText(player, "The great Ramires just granted you another permanent talent point!$B$BBut remember, you need to re-enter this world, level up or fulfill a quest to use and see it.$B$BDo you need some other service?");
        SendGossipMenuFor(player, LILLY_GOSSIP_TEXT_ID, creature->GetGUID());
    }

    // --- Goodbye messages ---
    static void HandleGoodbye(Player* player, Creature* creature)
    {
        uint32 goodbye = urand(1, 5);
        switch (goodbye)
        {
            case 1: creature->Say("Hey! Already enough of me?", LANG_UNIVERSAL); break;
            case 2: creature->TextEmote("Lilly shrugs her shoulders."); break;
            case 3: creature->Whisper("Do not forget. I will watch and rate your progress!", LANG_UNIVERSAL, player); break;
            case 4: creature->TextEmote("Lilly notes something on her imaginary clipboard."); break;
            default: creature->Say("Have a nice day then.", LANG_UNIVERSAL); break;
        }
        CloseGossipMenuFor(player);
    }
};

void AddSC_npc_lilly_gossip()
{
    new npc_custom_lilly();
}
