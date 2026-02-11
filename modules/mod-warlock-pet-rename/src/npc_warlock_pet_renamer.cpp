/*
 * Warlock Pet Renamer NPC — Gossip NPC for renaming Warlock demon pets.
 * Original: silviu20092
 * Fixes by Ramvars: Forced SMSG_PET_NAME_QUERY_RESPONSE for immediate client
 * name update, DirectExecute for synchronous DB write, improved dialogue.
 */

#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Pet.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "WorldPacket.h"

class npc_warlock_pet_renamer : public CreatureScript
{
private:
    static constexpr int VISUAL_FEEDBACK_SPELL_ID = 46331;

    static Pet* GetAllowedPetForRename(Player* player)
    {
        Pet* pet = player->GetPet();
        if (!pet)
            return nullptr;

        return pet->IsPet() && pet->GetOwnerGUID() == player->GetGUID() && pet->GetCharmInfo() != nullptr ? pet : nullptr;
    }

    static void NormalizeName(std::string& name)
    {
        std::transform(name.begin(), name.end(), name.begin(), tolower);
        name[0] = std::toupper(name[0]);
    }

    /// Force the client to immediately display the new pet name by sending
    /// SMSG_PET_NAME_QUERY_RESPONSE. Without this, the client may never re-query
    /// the name and the rename appears to "not work."
    static void SendPetNameUpdate(Player* player, Pet* pet, std::string const& name)
    {
        WorldPacket data(SMSG_PET_NAME_QUERY_RESPONSE, 4 + 4 + name.size() + 1 + 4);
        data << uint32(pet->GetCharmInfo()->GetPetNumber());
        data << name;
        data << uint32(pet->GetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP));
        data << uint8(0); // no declined name
        player->SendDirectMessage(&data);
    }

    static void HandlePetRename(Player* player, const char* nameStr)
    {
        Pet* pet = GetAllowedPetForRename(player);
        if (!pet)
            return;

        std::string name(nameStr);
        NormalizeName(name);

        PetNameInvalidReason res = ObjectMgr::CheckPetName(name);
        if (res != PET_NAME_SUCCESS)
        {
            player->GetSession()->SendPetNameInvalid(res, name, nullptr);
            return;
        }

        if (sObjectMgr->IsReservedName(name))
        {
            player->GetSession()->SendPetNameInvalid(PET_NAME_RESERVED, name, nullptr);
            return;
        }

        if (sObjectMgr->IsProfanityName(name))
        {
            player->GetSession()->SendPetNameInvalid(PET_NAME_PROFANE, name, nullptr);
            return;
        }

        pet->SetName(name);
        uint32 newTimestamp = uint32(GameTime::GetGameTime().count());
        pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, newTimestamp);

        // Visual sparkle effect
        player->CastSpell(pet, VISUAL_FEEDBACK_SPELL_ID, true);

        // Synchronous DB write — must complete before any menu refresh
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_PET_NAME);
        stmt->SetData(0, name);
        stmt->SetData(1, player->GetGUID().GetCounter());
        stmt->SetData(2, pet->GetCharmInfo()->GetPetNumber());
        CharacterDatabase.DirectExecute(stmt);

        // Force client to show the new name immediately
        SendPetNameUpdate(player, pet, name);

        ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00Your {} has been renamed to {}.|r", GetPetTypeName(pet), name);
    }

    static std::string GetPetTypeName(const Pet* pet)
    {
        switch (pet->GetEntry())
        {
            case NPC_IMP:        return "Imp";
            case NPC_VOIDWALKER: return "Voidwalker";
            case NPC_SUCCUBUS:   return "Succubus";
            case NPC_FELHUNTER:  return "Felhunter";
            case NPC_FELGUARD:   return "Felguard";
            case NPC_INFERNAL:   return "Infernal";
            case NPC_DOOMGUARD:  return "Doomguard";
            default:             return "Demon";
        }
    }

    static std::string GetPetInfo(const Pet* pet)
    {
        return pet->GetName() + " (" + GetPetTypeName(pet) + ")";
    }

public:
    npc_warlock_pet_renamer() : CreatureScript("npc_warlock_pet_renamer")
    {
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->getClass() != CLASS_WARLOCK)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cff999999I only work with Warlocks and their demons.|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "My apologies. Farewell.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }
        else
        {
            Pet* pet = GetAllowedPetForRename(player);
            if (!pet)
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffFFFF00Summon your demon first, then we can talk names.|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I'll be back.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            }
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Current companion: " + GetPetInfo(pet), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "I want to rename my demon.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3, "Enter the new name for your demon:", 0, true);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Not today. Farewell.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            }
        }

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            ClearGossipMenuFor(player);
            return OnGossipHello(player, creature);
        }

        CloseGossipMenuFor(player);
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* /*creature*/, uint32 /*sender*/ , uint32 action, const char* code) override
    {
        if (action == GOSSIP_ACTION_INFO_DEF + 3)
            HandlePetRename(player, code);

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_npc_warlock_pet_renamer()
{
    new npc_warlock_pet_renamer();
}
