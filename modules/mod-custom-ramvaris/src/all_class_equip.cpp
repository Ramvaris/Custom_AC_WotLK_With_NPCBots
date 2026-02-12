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
#include <unordered_set>

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
    // =====================================================================
    // Phase 2: Quests — open all class-restricted quests for cross-class
    // content.  Skill-learn quests (totems, stances, taming, forms, etc.)
    // are protected by a separate SQL patch that explicitly sets their
    // AllowableClasses — see:
    //   data/sql/db-world/9999_99_99_all_class_equip_lock_skill_quests.sql
    //
    // This C++ patcher opens everything ELSE each startup, so new quests
    // from upstream DB patches also get opened automatically.
    // =====================================================================
    static void PatchQuestRequirements()
    {
        // Step 1: DB — open all class-restricted quests that are NOT in our
        // explicit skill-quest lock list (i.e. quests whose AllowableClasses
        // was set by the base game, not by our SQL patch).
        // Our SQL patch uses specific IDs, so we just bulk-clear everything
        // then re-apply the locks.
        WorldDatabase.DirectExecute(
            "UPDATE quest_template_addon SET AllowableClasses = 0 "
            "WHERE AllowableClasses != 0");

        // Re-apply skill-quest locks (same IDs as the SQL patch file).
        // This ensures they survive even if someone drops and reimports.
        WorldDatabase.DirectExecute(
            "UPDATE quest_template_addon SET AllowableClasses = 64 "
            "WHERE ID IN (1531, 1532, 9554)");                          // Shaman: Call of Air
        WorldDatabase.DirectExecute(
            "UPDATE quest_template_addon SET AllowableClasses = 4 "
            "WHERE ID IN (6082, 6085, 6088, 6102, 9485, 9593)");        // Hunter: Taming the Beast
        // NOTE: Warlock mount quests (4490 Felsteed, 7631 Dreadsteed) and
        // Paladin mount quests (1661, 7647, 9712, 9737) are intentionally left
        // OPEN for collectors — they reward summon mounts, not class skills.
        WorldDatabase.DirectExecute(
            "UPDATE quest_template_addon SET AllowableClasses = 16 "
            "WHERE ID IN ("
            "5634,5635,5636,5637,5638,5639,5640,"                       // Priest: Desperate Prayer
            "5642,5643,5680,"                                           // Priest: Shadowguard
            "5652,5654,5655,5656,5657,"                                 // Priest: Hex of Weakness
            "5658,5660,5661,5662,5663,10379,"                           // Priest: Touch of Weakness
            "5627,5628,5629,5630,5631,5632,5633,"                       // Priest: Stars of Elune
            "5672,5673,5674,5675,"                                      // Priest: Elune's Grace
            "5676,5677,5678,"                                           // Priest: Arcane Feedback
            "10376,10378,"                                              // Priest: Symbol of Hope, Consume Magic
            "6032)");                                                   // Priest: Sacred Cloth

        // Step 2: In-memory — patch the already-loaded quest templates to match.
        uint32 opened = 0;

        // Build a set of skill-quest IDs that must stay locked.
        static const std::unordered_set<uint32> skillQuestIds = {
            // Shaman totems
            1531, 1532, 9554,
            // Already locked by base game (kept here so we never open them):
            96, 1518, 1521, 1527, 9451, 9509, 9555,
            // Hunter taming
            6082, 6085, 6088, 6102, 9485, 9593,
            // Already locked: 6081, 6086, 6089, 6103, 9673, 9675
            6081, 6086, 6089, 6103, 9673, 9675,
            // Warlock demon summons (NOT mounts — Felsteed 4490 / Dreadsteed 7631 are open for collectors)
            1470, 1471, 1474, 1485, 1504, 1513, 1598, 1599, 1689, 1739, 1795, 7583, 8344, 9619,
            // Also locked: 1, 12687  (Kanrethad — warlock green fire)
            1, 12687,
            // Priest racial spells
            5634, 5635, 5636, 5637, 5638, 5639, 5640,                  // Desperate Prayer
            5642, 5643, 5680,                                           // Shadowguard
            5652, 5654, 5655, 5656, 5657,                               // Hex of Weakness
            5658, 5660, 5661, 5662, 5663, 10379,                        // Touch of Weakness
            5627, 5628, 5629, 5630, 5631, 5632, 5633,                   // Stars of Elune
            5672, 5673, 5674, 5675,                                     // Elune's Grace
            5676, 5677, 5678,                                           // Arcane Feedback
            10376, 10378,                                               // Symbol of Hope, Consume Magic
            6032,                                                       // Sacred Cloth
            // Already locked priest: 5217,5220,5223,5226,5230,5232,5234,5236,
            // 5641,5644,5645,5646,5647,5679,10377
            5217, 5220, 5223, 5226, 5230, 5232, 5234, 5236,
            5641, 5644, 5645, 5646, 5647, 5679, 10377,
            // Warrior stances — already locked in base game
            1498, 1665, 1678, 1683, 1719, 1819, 9582, 10350,
            // Paladin class skills (NOT mounts — 1661/7647/9712/9737 are mount quests, open for collectors)
            1652, 1785, 1788, 9600, 9685, 9691,
            // Mage — already locked in base game
            99, 421, 422, 423, 424, 1014, 7463, 9364, 12172, 12173, 12228, 13079,
            // Druid — already locked in base game
            31, 755, 772, 5061, 6001, 6002, 6125, 6130, 6741, 6781,
            7223, 7224, 7962, 8257, 11001,
        };

        auto const& questStore = sObjectMgr->GetQuestTemplates();
        for (auto const& [id, quest] : questStore)
        {
            if (quest->GetRequiredClasses() == 0)
                continue;  // already open

            if (skillQuestIds.count(id))
                continue;  // skill quest — keep locked

            static_cast<QuestPatcher*>(quest)->SetRequiredClasses(0);
            ++opened;
        }

        if (opened > 0)
            LOG_INFO("server.loading", "[AllClassEquip] Opened {} class-restricted quests (skill quests stay locked)", opened);
    }
};

void AddSC_all_class_equip()
{
    new AllClassEquip_WorldScript();
}
