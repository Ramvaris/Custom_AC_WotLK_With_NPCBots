/*
 * Plate/Mail STR→AGI Conversion for AGI classes
 * Config: CustomRamvaris.AllClassEquip.StrToAgi.Enable
 *
 * When an agility-primary class (Hunter, Shaman, Rogue, Druid) equips a plate
 * or mail armor piece that has Strength on it, the Strength bonus is converted
 * to Agility instead.  This makes STR-heavy plate/mail sets usable for AGI
 * classes in an AllClassEquip environment where everyone can wear every armor
 * type.
 *
 * Hooks the OnPlayerApplyItemModsBefore callback which fires for each item stat
 * line during both equip (apply=true) and unequip (apply=false), so the
 * conversion is fully symmetrical — no manual tracking needed.
 *
 * Note: Does NOT touch weapon stats, ring/trinket/cloak stats, or items that
 * are cloth/leather.  Only class=ITEM_CLASS_ARMOR + subclass Plate(6) / Mail(4).
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "ItemTemplate.h"

class plate_mail_str_to_agi : public PlayerScript
{
public:
    plate_mail_str_to_agi() : PlayerScript("plate_mail_str_to_agi") { }

    void OnPlayerApplyItemModsBefore(Player* player, uint8 slot, bool apply,
                                     uint8 /*itemProtoStatNumber*/,
                                     uint32 statType, int32& val) override
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true))
            return;
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.AllClassEquip.Enable", false))
            return;
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.AllClassEquip.StrToAgi.Enable", true))
            return;

        // Only interested in Strength stats
        if (statType != ITEM_MOD_STRENGTH || val == 0)
            return;

        // Only for AGI-primary classes
        if (!IsAgilityClass(player->getClass()))
            return;

        // Only for plate or mail ARMOR pieces (not weapons, not rings/cloaks)
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            return;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto || proto->Class != ITEM_CLASS_ARMOR)
            return;

        // SubClass: 4 = Mail, 6 = Plate
        if (proto->SubClass != ITEM_SUBCLASS_ARMOR_MAIL &&
            proto->SubClass != ITEM_SUBCLASS_ARMOR_PLATE)
            return;

        // Convert: suppress the STR, manually apply the same amount as AGI
        int32 strVal = val;
        val = 0; // zero out STR so the engine doesn't apply it

        player->HandleStatFlatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE,
                                       float(strVal), apply);
        player->UpdateStatBuffMod(STAT_AGILITY);
    }

private:
    static bool IsAgilityClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_HUNTER:
            case CLASS_SHAMAN:  // Enhancement shaman benefits from AGI
            case CLASS_ROGUE:
            case CLASS_DRUID:
                return true;
            default:
                return false;
        }
    }
};

void AddSC_plate_mail_str_to_agi()
{
    new plate_mail_str_to_agi();
}
