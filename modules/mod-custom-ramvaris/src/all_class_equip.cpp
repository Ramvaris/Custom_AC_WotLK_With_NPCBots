/*
 * All-Class Equipment — WorldScript startup hook
 * Config: CustomRamvaris.AllClassEquip.Enable
 *
 * Runs on EVERY server startup AFTER all DB updates and item/quest loading.
 * This guarantees that new items added by upstream DB patches always get
 * AllowableClass = -1 applied, even if the initial SQL migration has already run.
 *
 * Two-phase approach:
 *  1. DB UPDATE — ensures the data persists across restarts
 *  2. In-memory patch — fixes the already-loaded templates for THIS boot
 *
 * Also removes class restrictions from quests that reward items (for transmog,
 * cross-class item collection) while preserving class locks on spell-only quests
 * (class trainers, talent unlock quests, etc.).
 */

#include "ScriptMgr.h"
#include "Config.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "QuestDef.h"
#include "ItemTemplate.h"
#include "SpellMgr.h"
#include "SpellInfo.h"

// Quest::RequiredClasses is protected with no setter.
// This derived class provides write access for the startup patcher.
class QuestPatcher : public Quest
{
public:
    void SetRequiredClasses(uint32 mask) { RequiredClasses = mask; }
};

class AllClassEquip_WorldScript : public WorldScript
{
public:
    AllClassEquip_WorldScript() : WorldScript("AllClassEquip_WorldScript") { }

    void OnStartup() override
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true))
            return;
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.AllClassEquip.Enable", false))
            return;

        PatchItemTemplates();
        PatchQuestRequirements();
    }

private:
    // =====================================================================
    // Phase 1: Weapons + Armor — AllowableClass = -1
    // =====================================================================
    static void PatchItemTemplates()
    {
        // --- DB: ensure future restarts see -1 for any new items ---
        WorldDatabase.DirectExecute(
            "UPDATE item_template SET AllowableClass = -1 "
            "WHERE class IN (2, 4) AND AllowableClass != -1 AND AllowableClass != 0");

        // --- In-memory: patch the already-loaded ItemTemplateContainer ---
        uint32 count = 0;
        ItemTemplateContainer const* store = sObjectMgr->GetItemTemplateStore();
        for (auto& [id, tmpl] : *const_cast<ItemTemplateContainer*>(store))
        {
            // class 2 = ITEM_CLASS_WEAPON, class 4 = ITEM_CLASS_ARMOR
            if ((tmpl.Class == ITEM_CLASS_WEAPON || tmpl.Class == ITEM_CLASS_ARMOR)
                && tmpl.AllowableClass != static_cast<uint32>(-1)
                && tmpl.AllowableClass != 0)
            {
                tmpl.AllowableClass = static_cast<uint32>(-1);
                ++count;
            }
        }

        if (count > 0)
            LOG_INFO("server.loading", "[AllClassEquip] Patched {} item templates (AllowableClass -> -1)", count);
        else
            LOG_INFO("server.loading", "[AllClassEquip] All item templates already unrestricted");
    }

    // =====================================================================
    // Phase 2: Quests — open all class-restricted quests EXCEPT those that
    // teach class-specific spells.  A spell is class-specific when its
    // SpellFamilyName maps to a single class (e.g. 11 = Shaman).
    // This keeps shaman totem quests, druid form quests, warlock demon
    // quests etc. locked to the correct class regardless of whether they
    // originally had AllowableClasses set or not.
    // =====================================================================
    static void PatchQuestRequirements()
    {
        uint32 opened = 0;
        uint32 locked = 0;
        auto const& questStore = sObjectMgr->GetQuestTemplates();

        for (auto const& [id, quest] : questStore)
        {
            // Check if this quest rewards a class-specific spell
            uint32 classMask = GetRewardSpellClassMask(quest);

            if (classMask != 0)
            {
                // This quest teaches a class skill — ensure it's locked.
                uint32 current = quest->GetRequiredClasses();
                if (current != classMask)
                {
                    static_cast<QuestPatcher*>(quest)->SetRequiredClasses(classMask);
                    // Persist to DB
                    WorldDatabase.DirectExecute(
                        "INSERT INTO quest_template_addon (ID, AllowableClasses) VALUES ({}, {}) "
                        "ON DUPLICATE KEY UPDATE AllowableClasses = {}",
                        id, classMask, classMask);
                    ++locked;
                }
            }
            else
            {
                // No class-specific spell reward — open it.
                if (quest->GetRequiredClasses() != 0)
                {
                    static_cast<QuestPatcher*>(quest)->SetRequiredClasses(0);
                    WorldDatabase.DirectExecute(
                        "UPDATE quest_template_addon SET AllowableClasses = 0 WHERE ID = {}", id);
                    ++opened;
                }
            }
        }

        if (opened > 0)
            LOG_INFO("server.loading", "[AllClassEquip] Opened {} class-restricted quests", opened);
        if (locked > 0)
            LOG_INFO("server.loading", "[AllClassEquip] Locked {} quests to their spell's class", locked);
    }

    // Map SpellFamilyName → class bitmask (1 << (classId - 1))
    static uint32 SpellFamilyToClassMask(uint32 family)
    {
        switch (family)
        {
            case  3: return 1 << (CLASS_MAGE         - 1);  // SPELLFAMILY_MAGE
            case  4: return 1 << (CLASS_WARRIOR      - 1);  // SPELLFAMILY_WARRIOR
            case  5: return 1 << (CLASS_WARLOCK      - 1);  // SPELLFAMILY_WARLOCK
            case  6: return 1 << (CLASS_PRIEST       - 1);  // SPELLFAMILY_PRIEST
            case  7: return 1 << (CLASS_DRUID        - 1);  // SPELLFAMILY_DRUID
            case  8: return 1 << (CLASS_ROGUE        - 1);  // SPELLFAMILY_ROGUE
            case  9: return 1 << (CLASS_HUNTER       - 1);  // SPELLFAMILY_HUNTER
            case 10: return 1 << (CLASS_PALADIN      - 1);  // SPELLFAMILY_PALADIN
            case 11: return 1 << (CLASS_SHAMAN       - 1);  // SPELLFAMILY_SHAMAN
            case 15: return 1 << (CLASS_DEATH_KNIGHT - 1);  // SPELLFAMILY_DEATHKNIGHT
            default: return 0; // Generic / environment / pet
        }
    }

    // Returns class mask if the quest's reward spell is class-specific, 0 otherwise.
    static uint32 GetRewardSpellClassMask(Quest const* quest)
    {
        uint32 spells[] = { uint32(quest->GetRewSpellCast()), quest->GetRewSpell() };
        for (uint32 spellId : spells)
        {
            if (spellId == 0)
                continue;
            SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
            if (!info)
                continue;
            uint32 mask = SpellFamilyToClassMask(info->SpellFamilyName);
            if (mask != 0)
                return mask;
            // Also check if the spell teaches another spell (SPELL_EFFECT_LEARN_SPELL)
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (info->Effects[i].Effect == SPELL_EFFECT_LEARN_SPELL && info->Effects[i].TriggerSpell)
                {
                    SpellInfo const* taught = sSpellMgr->GetSpellInfo(info->Effects[i].TriggerSpell);
                    if (taught)
                    {
                        mask = SpellFamilyToClassMask(taught->SpellFamilyName);
                        if (mask != 0)
                            return mask;
                    }
                }
            }
        }
        return 0;
    }
};

void AddSC_all_class_equip()
{
    new AllClassEquip_WorldScript();
}
