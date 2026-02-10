/*
 * mod-custom-ramvaris — Random Enchants System (Diablo-style class-specific)
 *
 * Originally based on mod-random-enchants by AzerothCore community (MIT License).
 * Completely rewritten for Ramvaris' server:
 * - Class-specific primary stat filtering (no useless stats for your class)
 * - All WotLK secondary stats in the pool (haste, hit, expertise, ArP, spell pen)
 * - Pure C++ enchant pools built from SpellItemEnchantment.dbc at startup
 * - No SQL tables, no broken queries, no stale data
 * - Up to 3 enchants per item using PROP_ENCHANTMENT_SLOT_0/1/2
 * - No conflict with player enchants (PERM slot), temp enchants, or sockets
 * - Items with existing random properties are skipped (already have stats)
 *
 * Original: https://github.com/azerothcore/mod-random-enchants (MIT License)
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Item.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Log.h"
#include "ItemTemplate.h"

// =============================================================================
// Enchant Pool — Built from DBC at server startup
// Groups pure-stat enchantments by stat type and value tier for fast O(1) lookup.
// =============================================================================

// Tier boundaries for enchant values — maps item quality to appropriate stat ranges.
// Tier 1 = weak (grey/white), Tier 5 = strongest (epic/legendary)
enum EnchantTier : uint8
{
    TIER_1 = 1,  // +1  to +5
    TIER_2 = 2,  // +6  to +12
    TIER_3 = 3,  // +13 to +22
    TIER_4 = 4,  // +23 to +35
    TIER_5 = 5,  // +36+
    MAX_TIER = 5
};

// Stats we care about — everything else is filtered out
enum AllowedStat : uint32
{
    STAT_STR   = ITEM_MOD_STRENGTH,           // 4
    STAT_AGI   = ITEM_MOD_AGILITY,            // 3
    STAT_INT   = ITEM_MOD_INTELLECT,          // 5
    STAT_SPI   = ITEM_MOD_SPIRIT,             // 6
    STAT_STA   = ITEM_MOD_STAMINA,            // 7
    STAT_SP    = ITEM_MOD_SPELL_POWER,        // 45
    STAT_HIT   = ITEM_MOD_HIT_RATING,         // 31
    STAT_CRIT  = ITEM_MOD_CRIT_RATING,        // 32
    STAT_HASTE = ITEM_MOD_HASTE_RATING,       // 36
    STAT_EXP   = ITEM_MOD_EXPERTISE_RATING,   // 37
    STAT_ARP   = ITEM_MOD_ARMOR_PENETRATION_RATING, // 44
    STAT_SPEN  = ITEM_MOD_SPELL_PENETRATION,  // 47
    STAT_DEF   = ITEM_MOD_DEFENSE_SKILL_RATING, // 12
    STAT_DODGE = ITEM_MOD_DODGE_RATING,       // 13
    STAT_PARRY = ITEM_MOD_PARRY_RATING,       // 14
    STAT_RES   = ITEM_MOD_RESILIENCE_RATING,  // 35
};

// Class-to-primary-stat mapping.
// Each class gets its PRIMARY stats + Stamina. Hybrid classes get ALL their primary stats.
// Spirit is ONLY primary for Priest.
// Attack Power / Ranged Attack Power are NOT in the pool (they come from primary stats).
static const std::unordered_map<uint8, std::vector<uint32>> CLASS_PRIMARY_STATS = {
    // CLASS_WARRIOR = 1: STR
    { 1,  { STAT_STR, STAT_STA } },
    // CLASS_PALADIN = 2: STR + INT (Holy/Ret/Prot all scale differently but both are used)
    { 2,  { STAT_STR, STAT_INT, STAT_STA } },
    // CLASS_HUNTER = 3: AGI
    { 3,  { STAT_AGI, STAT_STA } },
    // CLASS_ROGUE = 4: AGI
    { 4,  { STAT_AGI, STAT_STA } },
    // CLASS_PRIEST = 5: INT + SPI (only class where Spirit is primary — SP from Spirit talent)
    { 5,  { STAT_INT, STAT_SPI, STAT_STA } },
    // CLASS_DEATH_KNIGHT = 6: STR
    { 6,  { STAT_STR, STAT_STA } },
    // CLASS_SHAMAN = 7: INT + AGI (Enhance wants AGI, Ele/Resto want INT)
    { 7,  { STAT_INT, STAT_AGI, STAT_STA } },
    // CLASS_MAGE = 8: INT
    { 8,  { STAT_INT, STAT_STA } },
    // CLASS_WARLOCK = 9: INT
    { 9,  { STAT_INT, STAT_STA } },
    // CLASS_DRUID = 11: INT + AGI + STR (Balance/Resto=INT, Feral=AGI+STR, true hybrid)
    { 11, { STAT_INT, STAT_AGI, STAT_STR, STAT_STA } },
};

// Secondary stats available to ALL classes
static const std::vector<uint32> SECONDARY_STATS = {
    STAT_SP,     // Spell Power (compressed — no subtypes)
    STAT_HIT,    // Hit Rating
    STAT_CRIT,   // Crit Rating
    STAT_HASTE,  // Haste Rating
    STAT_EXP,    // Expertise Rating
    STAT_ARP,    // Armor Penetration Rating
    STAT_SPEN,   // Spell Penetration
    STAT_DEF,    // Defense Rating
    STAT_DODGE,  // Dodge Rating
    STAT_PARRY,  // Parry Rating
    STAT_RES,    // Resilience Rating
};

// Pool storage: stat type → tier → list of enchant IDs
// Built once at startup from DBC, never modified at runtime. Thread-safe reads.
static std::unordered_map<uint32, std::unordered_map<uint8, std::vector<uint32>>> s_enchantPools;
static bool s_poolsBuilt = false;

// Determine enchant tier from stat value
static EnchantTier GetTierFromValue(uint32 value)
{
    if (value <= 5)  return TIER_1;
    if (value <= 12) return TIER_2;
    if (value <= 22) return TIER_3;
    if (value <= 35) return TIER_4;
    return TIER_5;
}

// Map item quality to the enchant tier range it can roll
static EnchantTier GetMaxTierForQuality(uint32 quality)
{
    switch (quality)
    {
        case ITEM_QUALITY_POOR:      return TIER_1;
        case ITEM_QUALITY_NORMAL:    return TIER_2;
        case ITEM_QUALITY_UNCOMMON:  return TIER_3;
        case ITEM_QUALITY_RARE:      return TIER_4;
        case ITEM_QUALITY_EPIC:
        case ITEM_QUALITY_LEGENDARY: return TIER_5;
        default:                     return TIER_3;
    }
}

// Check if a stat type is one we accept for enchant pools
static bool IsAllowedStatType(uint32 statType)
{
    switch (statType)
    {
        case STAT_STR: case STAT_AGI: case STAT_INT: case STAT_SPI: case STAT_STA:
        case STAT_SP: case STAT_HIT: case STAT_CRIT: case STAT_HASTE: case STAT_EXP:
        case STAT_ARP: case STAT_SPEN: case STAT_DEF: case STAT_DODGE: case STAT_PARRY:
        case STAT_RES:
            return true;
        default:
            return false;
    }
}

// =============================================================================
// Build enchant pools from SpellItemEnchantment.dbc at startup.
// Filters for PURE single-stat enchantments only (type STAT, other effects NONE).
// =============================================================================
static void BuildEnchantPoolsFromDBC()
{
    if (s_poolsBuilt)
        return;

    uint32 totalFound = 0;
    uint32 totalSkipped = 0;

    for (uint32 id = 1; id < sSpellItemEnchantmentStore.GetNumRows(); ++id)
    {
        SpellItemEnchantmentEntry const* entry = sSpellItemEnchantmentStore.LookupEntry(id);
        if (!entry)
            continue;

        // Skip gem enchantments (GemID > 0 means this is a socket enchant)
        if (entry->GemID)
            continue;

        // Skip enchantments with conditions (socket color requirements, etc.)
        if (entry->EnchantmentCondition)
            continue;

        // Skip enchantments requiring specific skill (e.g., enchanting-only enchants)
        if (entry->requiredSkill)
            continue;

        // Find exactly ONE stat effect, rest must be NONE
        int statEffectIndex = -1;
        bool valid = true;

        for (uint32 e = 0; e < MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++e)
        {
            if (entry->type[e] == ITEM_ENCHANTMENT_TYPE_NONE)
                continue;

            if (entry->type[e] == ITEM_ENCHANTMENT_TYPE_STAT)
            {
                if (statEffectIndex >= 0)
                {
                    // Multiple stat effects — skip (multi-stat enchants are complex)
                    valid = false;
                    break;
                }
                statEffectIndex = e;
            }
            else
            {
                // Non-stat, non-none effect (proc, damage, resistance, etc.) — skip
                valid = false;
                break;
            }
        }

        if (!valid || statEffectIndex < 0)
        {
            ++totalSkipped;
            continue;
        }

        uint32 statType = entry->spellid[statEffectIndex]; // spellid field = effectArg = ITEM_MOD_*
        uint32 statValue = entry->amount[statEffectIndex];

        // Only accept stats we explicitly allow
        if (!IsAllowedStatType(statType))
        {
            ++totalSkipped;
            continue;
        }

        // Zero or negative values are useless
        if (statValue == 0)
        {
            ++totalSkipped;
            continue;
        }

        EnchantTier tier = GetTierFromValue(statValue);
        s_enchantPools[statType][tier].push_back(id);
        ++totalFound;
    }

    s_poolsBuilt = true;

    // Log pool statistics
    LOG_INFO("module", "RandomEnchants: Built enchant pools from DBC — {} enchants in {} stat types, {} skipped",
             totalFound, s_enchantPools.size(), totalSkipped);

    for (auto const& [stat, tiers] : s_enchantPools)
    {
        uint32 total = 0;
        for (auto const& [tier, ids] : tiers)
            total += ids.size();
        LOG_INFO("module", "  Stat {}: {} enchants across {} tiers", stat, total, tiers.size());
    }
}

// =============================================================================
// Get a random enchant ID appropriate for the player's class and item quality.
// Returns 0 if no valid enchant found.
// =============================================================================
static uint32 GetClassAppropriateEnchant(Player* player, Item* item, uint32 excludeEnchant1 = 0, uint32 excludeEnchant2 = 0)
{
    if (!player || !item)
        return 0;

    uint8 playerClass = player->getClass();
    uint32 quality = item->GetTemplate()->Quality;
    EnchantTier maxTier = GetMaxTierForQuality(quality);

    // Build the list of allowed stats for this class
    std::vector<uint32> allowedStats;

    // Add class primary stats
    auto classIt = CLASS_PRIMARY_STATS.find(playerClass);
    if (classIt != CLASS_PRIMARY_STATS.end())
    {
        for (uint32 stat : classIt->second)
            allowedStats.push_back(stat);
    }

    // Add universal secondary stats
    for (uint32 stat : SECONDARY_STATS)
        allowedStats.push_back(stat);

    // Shuffle to randomize which stat category gets checked first
    Acore::Containers::RandomShuffle(allowedStats);

    // Collect all valid enchant IDs for this class + quality
    std::vector<uint32> validEnchants;
    validEnchants.reserve(64);

    for (uint32 stat : allowedStats)
    {
        auto poolIt = s_enchantPools.find(stat);
        if (poolIt == s_enchantPools.end())
            continue;

        // Roll a tier — higher quality items can get higher tiers
        // Use weighted distribution: higher tiers more likely for better items
        for (uint8 t = 1; t <= maxTier; ++t)
        {
            auto tierIt = poolIt->second.find(t);
            if (tierIt == poolIt->second.end())
                continue;

            for (uint32 enchantId : tierIt->second)
            {
                // Exclude duplicates from previous slots
                if (enchantId == excludeEnchant1 || enchantId == excludeEnchant2)
                    continue;
                validEnchants.push_back(enchantId);
            }
        }
    }

    if (validEnchants.empty())
        return 0;

    // Weight selection: prefer higher tiers for higher quality items
    // Simple approach: just pick random from the valid pool (tiers already filtered by maxTier)
    return validEnchants[urand(0, validEnchants.size() - 1)];
}

// =============================================================================
// Roll and apply enchants to an item.
// Up to 3 enchants in PROP_ENCHANTMENT_SLOT_0/1/2 (don't conflict with player enchants).
// Items that already have random properties (RandomProperty/RandomSuffix) are skipped.
// =============================================================================
static void RollPossibleEnchant(Player* player, Item* item)
{
    if (!player || !item || !item->IsInWorld())
        return;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return;

    // Only enchant weapons (class 2) and armor (class 4)
    if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
        return;

    // Skip items with quality below Normal (poor) or above Legendary
    if (proto->Quality < ITEM_QUALITY_POOR || proto->Quality > ITEM_QUALITY_LEGENDARY)
        return;

    // Skip items that already have random properties (native "of the Bear" etc.)
    // These items already use PROP_ENCHANTMENT_SLOT and we don't want to overwrite them.
    if (item->GetItemRandomPropertyId() != 0)
        return;

    // Cascading chance: must succeed #1 to roll #2, must succeed #2 to roll #3
    // Default: 70% / 65% / 60% — same as original mod-random-enchants
    constexpr float CHANCE_1 = 70.0f;
    constexpr float CHANCE_2 = 65.0f;
    constexpr float CHANCE_3 = 60.0f;

    uint32 enchants[3] = { 0, 0, 0 };
    static const EnchantmentSlot SLOTS[3] = {
        PROP_ENCHANTMENT_SLOT_0, // 7
        PROP_ENCHANTMENT_SLOT_1, // 8
        PROP_ENCHANTMENT_SLOT_2  // 9
    };

    // Roll enchant 1
    if (rand_chance() < CHANCE_1)
    {
        enchants[0] = GetClassAppropriateEnchant(player, item);
    }

    // Roll enchant 2 (only if #1 succeeded)
    if (enchants[0] && rand_chance() < CHANCE_2)
    {
        enchants[1] = GetClassAppropriateEnchant(player, item, enchants[0]);
    }

    // Roll enchant 3 (only if #2 succeeded)
    if (enchants[1] && rand_chance() < CHANCE_3)
    {
        enchants[2] = GetClassAppropriateEnchant(player, item, enchants[0], enchants[1]);
    }

    // Apply enchants to item
    uint32 appliedCount = 0;
    for (uint8 i = 0; i < 3; ++i)
    {
        if (enchants[i] == 0)
            continue;

        // Validate - does this enchant ID exist in DBC?
        SpellItemEnchantmentEntry const* enchEntry = sSpellItemEnchantmentStore.LookupEntry(enchants[i]);
        if (!enchEntry)
            continue;

        // Remove old enchant in this slot (if any) and apply new one
        player->ApplyEnchantment(item, SLOTS[i], false);
        item->SetEnchantment(SLOTS[i], enchants[i], 0, 0);
        player->ApplyEnchantment(item, SLOTS[i], true);
        ++appliedCount;
    }

    // Notify the player
    if (appliedCount > 0)
    {
        int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        std::string itemName = proto->Name1;
        if (ItemLocale const* il = sObjectMgr->GetItemLocale(proto->ItemId))
            ObjectMgr::GetLocaleString(il->Name, loc_idx, itemName);

        if (appliedCount == 1)
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffFF8000%s|r hat |cff00FF001|r zufällige Verzauberung erhalten!", itemName.c_str());
        else
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffFF8000%s|r hat |cff00FF00%u|r zufällige Verzauberungen erhalten!", itemName.c_str(), appliedCount);
    }
}

// =============================================================================
// PlayerScript: Hooks into loot, craft, quest reward, group roll
// Replaces the old mod-random-enchants module entirely.
// =============================================================================
class RandomEnchants_PlayerScript : public PlayerScript
{
public:
    RandomEnchants_PlayerScript() : PlayerScript("RandomEnchants_PlayerScript", {
        PLAYERHOOK_ON_LOOT_ITEM,
        PLAYERHOOK_ON_CREATE_ITEM,
        PLAYERHOOK_ON_QUEST_REWARD_ITEM,
        PLAYERHOOK_ON_GROUP_ROLL_REWARD_ITEM
    }) { }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override
    {
        if (!IsEnabled()) return;
        RollPossibleEnchant(player, item);
    }

    void OnPlayerCreateItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (!IsEnabled()) return;
        RollPossibleEnchant(player, item);
    }

    void OnPlayerQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (!IsEnabled()) return;
        RollPossibleEnchant(player, item);
    }

    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override
    {
        if (!IsEnabled()) return;
        RollPossibleEnchant(player, item);
    }

private:
    static bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
               sConfigMgr->GetOption<bool>("CustomRamvaris.RandomEnchants.Enable", false);
    }
};

// =============================================================================
// WorldScript: Build enchant pools from DBC when the server starts
// =============================================================================
class RandomEnchants_WorldScript : public WorldScript
{
public:
    RandomEnchants_WorldScript() : WorldScript("RandomEnchants_WorldScript") { }

    void OnStartup() override
    {
        if (sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
            sConfigMgr->GetOption<bool>("CustomRamvaris.RandomEnchants.Enable", false))
        {
            BuildEnchantPoolsFromDBC();
        }
    }
};

// =============================================================================
// Script Registration
// =============================================================================
void AddSC_random_enchants()
{
    new RandomEnchants_PlayerScript();
    new RandomEnchants_WorldScript();
}
