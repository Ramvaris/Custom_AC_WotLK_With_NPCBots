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

// Quest::RequiredClasses is protected with no setter.
// This derived class provides write access for the startup patcher.
class QuestPatcher : public Quest
{
public:
    void ClearRequiredClasses() { RequiredClasses = 0; }
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
        EnsureAgiPlateItems();
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
    // Phase 2: Quests — remove class restrictions on item-reward quests
    // Preserves class locks on quests that ONLY grant spells (trainers).
    // Hybrid quests (items + spells) are opened — harmless extra spell.
    // =====================================================================
    static void PatchQuestRequirements()
    {
        // --- DB: clear AllowableClasses on quests that give item rewards ---
        // Subquery finds quest IDs that have at least one item reward.
        // Quests with ONLY spell rewards (no item rewards) are left alone.
        WorldDatabase.DirectExecute(
            "UPDATE quest_template_addon qta "
            "JOIN quest_template qt ON qta.ID = qt.ID "
            "SET qta.AllowableClasses = 0 "
            "WHERE qta.AllowableClasses != 0 "
            "  AND (qt.RewardItem1 != 0 OR qt.RewardItem2 != 0 "
            "       OR qt.RewardItem3 != 0 OR qt.RewardItem4 != 0 "
            "       OR qt.RewardChoiceItemID1 != 0 OR qt.RewardChoiceItemID2 != 0 "
            "       OR qt.RewardChoiceItemID3 != 0 OR qt.RewardChoiceItemID4 != 0 "
            "       OR qt.RewardChoiceItemID5 != 0 OR qt.RewardChoiceItemID6 != 0)");

        // --- In-memory: patch loaded quest templates ---
        uint32 count = 0;
        auto const& questStore = sObjectMgr->GetQuestTemplates();
        for (auto const& [id, quest] : questStore)
        {
            if (quest->GetRequiredClasses() == 0)
                continue;

            // Check if this quest gives any item rewards
            bool hasItemReward = false;
            for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            {
                if (quest->RewardItemId[i] != 0)
                {
                    hasItemReward = true;
                    break;
                }
            }
            if (!hasItemReward)
            {
                for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
                {
                    if (quest->RewardChoiceItemId[i] != 0)
                    {
                        hasItemReward = true;
                        break;
                    }
                }
            }

            if (hasItemReward)
            {
                static_cast<QuestPatcher*>(quest)->ClearRequiredClasses();
                ++count;
            }
        }

        if (count > 0)
            LOG_INFO("server.loading", "[AllClassEquip] Opened {} class-restricted quests (item rewards)", count);
        else
            LOG_INFO("server.loading", "[AllClassEquip] All item-reward quests already unrestricted");
    }

    // =====================================================================
    // Phase 3: Ensure custom AGI plate items exist
    // These are created by 9999_99_99_agi_plate_sets.sql on first run.
    // This check logs whether they loaded successfully — if item_dbc entries
    // are missing (pre-DBC-load, can't fix at runtime), it logs a warning.
    // The item_template and npc_vendor rows are re-ensured via the SQL file.
    // =====================================================================
    static void EnsureAgiPlateItems()
    {
        uint32 found = 0;
        for (uint32 entry = 80001; entry <= 80010; ++entry)
        {
            if (sObjectMgr->GetItemTemplate(entry))
                ++found;
        }

        if (found == 10)
            LOG_INFO("server.loading", "[AllClassEquip] AGI plate sets loaded (10/10 items: Nightstalker + Phantom Dreadnaught)");
        else if (found > 0)
            LOG_WARN("server.loading", "[AllClassEquip] AGI plate sets partially loaded ({}/10) — check item_dbc entries", found);
        else
            LOG_WARN("server.loading", "[AllClassEquip] AGI plate sets NOT loaded — item_dbc entries may be missing. Re-run 9999_99_99_agi_plate_sets.sql");
    }
};

void AddSC_all_class_equip()
{
    new AllClassEquip_WorldScript();
}
