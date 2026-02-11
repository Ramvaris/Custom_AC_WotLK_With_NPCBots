/*
 * mod-custom-ramvaris — Lottery Enchant System (Diablo-style)
 *
 * Originally based on mod-random-enchants by AzerothCore community (MIT License).
 * Completely rewritten for Ramvaris' server:
 *
 * ARCHITECTURE:
 * - Enchants stored in custom DB table `character_item_lottery_enchants`, NOT in item slots
 * - Server applies stats on equip/login, unapplies on unequip/logout
 * - Up to 7 enchants per item, cascading 70/60/50/50/50/40/40% chances (~1.47% for 7)
 * - Stats are LEVEL-SCALED: max(1, round(base * level / 80)) — grows with the player
 * - NO class filtering — any stat/resist can roll on any class. Self-balancing: a warrior
 *   rolling INT+SPI+FIRE_RES is a "crap roll". The dilution IS the balance mechanism.
 * - Pool includes: primary stats (STR/AGI/INT/SPI/STA), combat ratings, spell power,
 *   haste, crit, hit, expertise, armor pen, resilience, AND all elemental resistances.
 * - Pool EXCLUDES: Dodge, Parry, Defense (overcapping with multiple items)
 * CUSTOM ENCHANTS: Movespeed (1-25% per roll, cap +100% = 200% base, stacking,
 *   affects run + swim + flight, NOT level-scaled — 10% rolled = 10% at any level)
 *   and Mobility Boost (1% chance, 3 stages: 100%/200%/300% flight speed, combined
 *   with speed bonus up to 600%, NO level scaling).
 * - `.enchants` paginated gossip menu; `.reroll` gossip-based Keep/Take flow (500g)
 * - `.flylock` locks full flight, switches to double jump mode
 * - NO quality-based tier filtering — the percentile system handles balance naturally.
 *   A grey item can roll god-tier stats if RNG blesses you. Any quality, any tier.
 * - 7 color tiers by percentile (~14.29% each): Grey→White→Green→Blue→Purple→Orange→Red
 * - Item name color = enchant COUNT (1=grey..7=red), stat color = VALUE percentile
 * - Items with lottery enchants get 777/777 durability as visual marker
 * - Stats show in character sheet, item tooltip untouched (no client modification)
 *
 * STACKING WITH PROFESSION ENCHANTS:
 * Our stats are applied via HandleStatFlatModifier/ApplyRatingMod directly, NOT via
 * item enchantment slots. Profession enchants live in slot 0 (PERM), ours live in a
 * DB table. They stack additively — no aura conflict, no override, pure bonus math.
 *
 * EQUIP/UNEQUIP SAFETY:
 * Item swaps (right-click to equip from inventory) fire OnPlayerEquip for the new
 * item but do NOT always fire OnPlayerUnequip for the displaced item. Incremental
 * apply/unapply would leave orphaned bonuses → infinite stat stacking. We use a
 * full-recalc approach: on EVERY equip/unequip/login event, unapply all tracked
 * enchants and reapply only for items CURRENTLY in equipment slots. A per-player
 * tracking set (s_appliedItems) ensures we always know what to unapply.
 *
 * LEVEL SCALING:
 * DBC enchant values are the level-80 maximum. At lower levels, stats scale linearly:
 *   scaledAmount = max(1, round(baseAmount * playerLevel / 80))
 * Level 1 → 1.25%, Level 40 → 50%, Level 80 → 100%. Stats update on every level-up.
 * Speed and Fly enchants do NOT level-scale — their values are always the raw roll.
 *
 * SPEED/FLY MECHANIC:
 * Speed modifies base character speed (run, swim, AND flight), multiplicative with
 * aura buffs. E.g. +50% from enchants × 15% paladin aura = 1.5 × 1.15 = 172.5%.
 * Capped at +100% from enchants (200% base speed). Individual rolls are 1-25%.
 * NOT level-scaled — 10% rolled = 10% at any level, always the raw rolled value.
 * Swimming is treated the same as running.
 * MOUNTED PLAYERS GET NO SPEED BONUS — mounts use vanilla 100% base speed. This
 * intentionally makes mounts obsolete as enchant speed grows (epic mount = 200%,
 * but +100% enchant speed unmounted = 200% without needing a mount).
 *
 * MOBILITY BOOST (renamed from "Flying"):
 * 1% overall chance in the dice pool.
 * Sub-roll: 75%=Stage1 (100%), 20%=Stage2 (200%), 5%=Stage3 (300%). Only highest
 * stage counts across ALL equipped items. Effective fly speed = flyStageRate ×
 * (1 + speedBonus). Max: 3.0 × 2.0 = 600% flight speed. Works everywhere.
 * BG flag carriers auto-drop the flag when airborne.
 *
 * TWO MODES based on level and .flylock:
 *   FULL FLIGHT (level >= 60, .flylock OFF):
 *     SetCanFly(true) → flying mount behavior. On ground: space = jump.
 *     In air/falling: space = start flying. Sustained flight with stage speed.
 *   DOUBLE JUMP (level < 60 OR .flylock ON):
 *     SetCanFly(true) as trigger → when player presses space while falling, the
 *     client enters fly state → OnPlayerUpdate detects IsFlying() → applies upward
 *     KnockbackFrom(0, 15) → SetCanFly(false). Player arcs up then falls normally.
 *     One double jump per airborne session, resets on landing. Resets fall damage
 *     tracking to the activation point. Useful in boss fights where flying would
 *     risk boss reset.
 *   .flylock toggles between full flight and double jump for level 60+ players.
 *
 * ITEM SOURCES:
 * Uses PLAYERHOOK_ON_STORE_NEW_ITEM — catches ALL item acquisition (loot, craft, quest,
 * vendor purchase, mail, group roll). ALL item qualities (grey through legendary) get
 * enchants from non-vendor sources. Vendor purchases are detected via
 * PLAYERHOOK_ON_BEFORE_BUY_ITEM_FROM_VENDOR — only Green+ vendor items get enchants,
 * grey/white vendor trash is skipped to prevent mass-buying cheap items for farming.
 * Vendor buyback uses Player::StoreItem (not StoreNewItem), so repurchased items are safe.
 * Additionally, `.reroll` allows re-rolling bag items for 500g via gossip menu.
 *
 * Original: https://github.com/azerothcore/mod-random-enchants (MIT License)
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Item.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "DBCStores.h"
#include "Log.h"
#include "ItemTemplate.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "ScriptedGossip.h"
#include "WorldPacket.h"
#include "Opcodes.h"

using namespace Acore::ChatCommands;

// =============================================================================
// Constants
// =============================================================================

// 7 rarity tiers — each covers ~14.29% of the value range (1 to globalMax)
enum EnchantTier : uint8
{
    TIER_GREY   = 1,
    TIER_WHITE  = 2,
    TIER_GREEN  = 3,
    TIER_BLUE   = 4,
    TIER_PURPLE = 5,
    TIER_ORANGE = 6,
    TIER_RED    = 7,
    MAX_TIER    = 7
};

// Stats accepted for the lottery pool — everything else filtered out.
// Dodge/Parry/Defense REMOVED — overcaps with multiple items.
enum AllowedStat : uint32
{
    STAT_STR   = ITEM_MOD_STRENGTH,
    STAT_AGI   = ITEM_MOD_AGILITY,
    STAT_INT   = ITEM_MOD_INTELLECT,
    STAT_SPI   = ITEM_MOD_SPIRIT,
    STAT_STA   = ITEM_MOD_STAMINA,
    STAT_SP    = ITEM_MOD_SPELL_POWER,
    STAT_HIT   = ITEM_MOD_HIT_RATING,
    STAT_CRIT  = ITEM_MOD_CRIT_RATING,
    STAT_HASTE = ITEM_MOD_HASTE_RATING,
    STAT_EXP   = ITEM_MOD_EXPERTISE_RATING,
    STAT_ARP   = ITEM_MOD_ARMOR_PENETRATION_RATING,
    STAT_SPEN  = ITEM_MOD_SPELL_PENETRATION,
    STAT_RES   = ITEM_MOD_RESILIENCE_RATING,
};

// Synthetic pool keys for resistance enchants (ITEM_ENCHANTMENT_TYPE_RESISTANCE).
// Uses 1000 + SpellSchool to avoid collision with ITEM_MOD_* stat values.
static constexpr uint32 RESIST_POOL_KEY_BASE = 1000;

// Synthetic pool key for Movespeed in the pool-type selection.
// Not a real DBC stat — handled specially when selected.
static constexpr uint32 SPEED_POOL_KEY = 2000;

// Custom enchant IDs — stored in DB alongside real DBC enchant IDs.
// Range 900001+ chosen to never collide with real SpellItemEnchantment.dbc IDs.
// Speed: 900001–900025 = 1–25% movespeed. Fly: 900101/02/03 = stage 1/2/3.
static constexpr uint32 CUSTOM_ENCHANT_SPEED_BASE = 900001; // + (1..25) = pct
static constexpr uint32 CUSTOM_ENCHANT_FLY_BASE   = 900100; // + (1..3) = stage

// Gossip menu constants — must NOT collide with Soul Keeper (89999/8999)
// or custom_commands.cpp SPECIAL_GOSSIP_SENDER (99999).
static constexpr uint32 LOTTERY_GOSSIP_MENU_ID = 69999;
static constexpr uint32 LOTTERY_GOSSIP_SENDER  = 6999;
static constexpr uint32 LOTTERY_NPC_TEXT_ID    = 0x7FFFFFFE; // Distinct from other modules

// Gossip action ranges
enum LotteryGossipAction : uint32
{
    LOTTO_ACTION_CLOSE           = 0,
    LOTTO_ACTION_NEXT_PAGE       = 1,
    LOTTO_ACTION_PREV_PAGE       = 2,
    LOTTO_ACTION_FIRST_PAGE      = 3,
    LOTTO_ACTION_LAST_PAGE       = 4,

    // Reroll menu: select item (100 + local index on page)
    LOTTO_ACTION_REROLL_BASE     = 100,
    LOTTO_ACTION_REROLL_MAX      = 9999,

    // Reroll confirm/preview
    LOTTO_ACTION_REROLL_CONFIRM  = 10001,
    LOTTO_ACTION_REROLL_TAKE     = 10002,
    LOTTO_ACTION_REROLL_KEEP     = 10003,

    // Enchants viewer (20000 + local index on page → show details)
    LOTTO_ACTION_ENCHANTS_BASE   = 20000,
    LOTTO_ACTION_ENCHANTS_MAX    = 29999,
    LOTTO_ACTION_ENCHANTS_NEXT   = 30001,
    LOTTO_ACTION_ENCHANTS_PREV   = 30002,
    LOTTO_ACTION_ENCHANTS_FIRST  = 30003,
    LOTTO_ACTION_ENCHANTS_LAST   = 30004,
    LOTTO_ACTION_ENCHANTS_BACK   = 30005,
};

static constexpr uint32 ITEMS_PER_PAGE = 15;

// Cascading roll chances per enchant slot
static constexpr float ROLL_CHANCES[] = {
    70.0f, 60.0f, 50.0f, 50.0f, 50.0f, 40.0f, 40.0f
};
static constexpr uint32 MAX_LOTTERY_SLOTS = 7;
static constexpr uint32 REROLL_COST_GOLD  = 500;

// =============================================================================
// Static Globals
// =============================================================================

// Pool storage: stat/resist pool key → enchant IDs (built once from DBC)
static std::unordered_map<uint32, std::vector<uint32>> s_enchantPools;
static uint32 s_globalMaxEnchantValue = 1;
static bool s_poolsBuilt = false;

// In-memory cache: item GUID → list of enchant IDs (ordered by slot_index)
static std::unordered_map<uint32, std::vector<uint32>> s_lotteryCache;
static std::mutex s_lotteryCacheMutex;

// Pending reroll state: player GUID → { itemGuid, rolledEnchants }
struct PendingReroll
{
    uint32 itemGuid;
    std::vector<uint32> rolledEnchants;
};
static std::unordered_map<uint32, PendingReroll> s_pendingRerolls;

// Gossip page state: player GUID → current page (for reroll and enchants menus)
static std::unordered_map<uint32, uint32> s_rerollPage;
static std::unordered_map<uint32, uint32> s_enchantsPage;

// Enchants viewer: cached item list per player (rebuilt on menu open)
struct EnchantViewerItem
{
    uint32 itemGuid;
    uint32 itemEntry;
    uint32 enchantCount;
    bool   equipped;
    uint8  equipSlot;   // Equipment slot index (0xFF for bag items)
};
static std::unordered_map<uint32, std::vector<EnchantViewerItem>> s_enchantsItemList;

// Reroll menu: cached eligible items per player
struct RerollMenuItem
{
    uint32 itemGuid;
    uint32 itemEntry;
    uint32 enchantCount;
};
static std::unordered_map<uint32, std::vector<RerollMenuItem>> s_rerollItemList;

// Vendor purchase detection: player GUIDs currently in a BuyItemFromVendorSlot call.
// Set in OnPlayerBeforeBuyItemFromVendor, checked+cleared in OnPlayerStoreNewItem.
static std::unordered_set<uint32> s_vendorBuying;

// Equip tracking: player GUID → set of item GUIDs with currently applied enchants.
// Used to prevent double-applying on item swaps where OnPlayerUnequip doesn't fire.
static std::unordered_map<uint32, std::unordered_set<uint32>> s_appliedItems;

// Double jump tracking: player GUID → has used double jump this airborne session.
// Reset when the player lands on the ground. Only active for players with fly enchant
// who are in double-jump mode (level <60 or flylock ON).
static std::unordered_map<uint32, bool> s_doubleJumpUsed;

// =============================================================================
// Helper functions — Custom enchant ID detection
// =============================================================================

static bool IsCustomSpeedEnchant(uint32 id)
{
    return id >= CUSTOM_ENCHANT_SPEED_BASE && id <= CUSTOM_ENCHANT_SPEED_BASE + 25;
}

static bool IsCustomFlyEnchant(uint32 id)
{
    return id >= CUSTOM_ENCHANT_FLY_BASE + 1 && id <= CUSTOM_ENCHANT_FLY_BASE + 3;
}

static bool IsCustomEnchant(uint32 id)
{
    return IsCustomSpeedEnchant(id) || IsCustomFlyEnchant(id);
}

// Speed enchant: returns 1-25 (percentage)
static uint32 GetSpeedPct(uint32 id)
{
    return id - CUSTOM_ENCHANT_SPEED_BASE;
}

// Fly enchant: returns 1/2/3 (stage)
static uint32 GetFlyStage(uint32 id)
{
    return id - CUSTOM_ENCHANT_FLY_BASE;
}

// =============================================================================
// Helper functions — Tier, color, display names
// =============================================================================

static EnchantTier GetTierFromValue(uint32 value)
{
    if (s_globalMaxEnchantValue <= 1)
        return TIER_GREY;

    float pct = float(value) / float(s_globalMaxEnchantValue);
    if (pct <= 0.1429f) return TIER_GREY;
    if (pct <= 0.2857f) return TIER_WHITE;
    if (pct <= 0.4286f) return TIER_GREEN;
    if (pct <= 0.5714f) return TIER_BLUE;
    if (pct <= 0.7143f) return TIER_PURPLE;
    if (pct <= 0.8571f) return TIER_ORANGE;
    return TIER_RED;
}

static EnchantTier GetMaxTierForQuality(uint32 quality)
{
    switch (quality)
    {
        case ITEM_QUALITY_POOR:      return TIER_GREY;
        case ITEM_QUALITY_NORMAL:    return TIER_WHITE;
        case ITEM_QUALITY_UNCOMMON:  return TIER_GREEN;
        case ITEM_QUALITY_RARE:      return TIER_BLUE;
        case ITEM_QUALITY_EPIC:      return TIER_PURPLE;
        case ITEM_QUALITY_LEGENDARY: return TIER_RED;
        default:                     return TIER_GREEN;
    }
}

static bool IsAllowedStatType(uint32 statType)
{
    switch (statType)
    {
        case STAT_STR: case STAT_AGI: case STAT_INT: case STAT_SPI: case STAT_STA:
        case STAT_SP: case STAT_HIT: case STAT_CRIT: case STAT_HASTE: case STAT_EXP:
        case STAT_ARP: case STAT_SPEN: case STAT_RES:
            return true;
        default:
            return false;
    }
}

static bool IsAllowedResistSchool(uint32 school)
{
    return school >= 1 && school <= 6;
}

static const char* GetValueColor(EnchantTier tier)
{
    switch (tier)
    {
        case TIER_GREY:   return "|cffBBBBBB";
        case TIER_WHITE:  return "|cffFFFFFF";
        case TIER_GREEN:  return "|cff1EFF00";
        case TIER_BLUE:   return "|cff0070DD";
        case TIER_PURPLE: return "|cffA335EE";
        case TIER_ORANGE: return "|cffFF8000";
        case TIER_RED:    return "|cffFF0000";
        default:          return "|cffBBBBBB";
    }
}

static const char* GetCountColor(uint32 count)
{
    switch (count)
    {
        case 1:  return "|cffBBBBBB";
        case 2:  return "|cffFFFFFF";
        case 3:  return "|cff1EFF00";
        case 4:  return "|cff0070DD";
        case 5:  return "|cffA335EE";
        case 6:  return "|cffFF8000";
        default: return "|cffFF0000";
    }
}

static const char* GetStatName(uint32 statType)
{
    switch (statType)
    {
        case ITEM_MOD_MANA:                     return "Mana";
        case ITEM_MOD_HEALTH:                   return "Health";
        case ITEM_MOD_AGILITY:                  return "Agility";
        case ITEM_MOD_STRENGTH:                 return "Strength";
        case ITEM_MOD_INTELLECT:                return "Intellect";
        case ITEM_MOD_SPIRIT:                   return "Spirit";
        case ITEM_MOD_STAMINA:                  return "Stamina";
        case ITEM_MOD_DEFENSE_SKILL_RATING:     return "Defense";
        case ITEM_MOD_DODGE_RATING:             return "Dodge";
        case ITEM_MOD_PARRY_RATING:             return "Parry";
        case ITEM_MOD_BLOCK_RATING:             return "Block";
        case ITEM_MOD_HIT_MELEE_RATING:         return "Melee Hit";
        case ITEM_MOD_HIT_RANGED_RATING:        return "Ranged Hit";
        case ITEM_MOD_HIT_SPELL_RATING:         return "Spell Hit";
        case ITEM_MOD_CRIT_MELEE_RATING:        return "Melee Crit";
        case ITEM_MOD_CRIT_RANGED_RATING:       return "Ranged Crit";
        case ITEM_MOD_CRIT_SPELL_RATING:        return "Spell Crit";
        case ITEM_MOD_HIT_RATING:               return "Hit";
        case ITEM_MOD_CRIT_RATING:              return "Crit";
        case ITEM_MOD_RESILIENCE_RATING:        return "Resilience";
        case ITEM_MOD_HASTE_RATING:             return "Haste";
        case ITEM_MOD_EXPERTISE_RATING:         return "Expertise";
        case ITEM_MOD_ATTACK_POWER:             return "Attack Power";
        case ITEM_MOD_RANGED_ATTACK_POWER:      return "Ranged AP";
        case ITEM_MOD_ARMOR_PENETRATION_RATING: return "Armor Pen";
        case ITEM_MOD_SPELL_POWER:              return "Spell Power";
        case ITEM_MOD_SPELL_PENETRATION:        return "Spell Pen";
        case ITEM_MOD_MANA_REGENERATION:        return "MP5";
        case ITEM_MOD_HEALTH_REGEN:             return "HP5";
        case ITEM_MOD_BLOCK_VALUE:              return "Block Value";
        default:                                return "Unknown";
    }
}

static const char* GetResistName(uint32 school)
{
    switch (school)
    {
        case 1: return "Holy Resist";
        case 2: return "Fire Resist";
        case 3: return "Nature Resist";
        case 4: return "Frost Resist";
        case 5: return "Shadow Resist";
        case 6: return "Arcane Resist";
        default: return "Resist";
    }
}

// Resolve item name with locale
static std::string GetItemDisplayName(Player* player, ItemTemplate const* proto)
{
    std::string name = proto->Name1;
    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    if (ItemLocale const* il = sObjectMgr->GetItemLocale(proto->ItemId))
        ObjectMgr::GetLocaleString(il->Name, loc_idx, name);
    return name;
}

// Short equipment slot name for gossip labels (e.g., "Head", "MH", "Ring1")
static const char* GetEquipSlotShort(uint8 slot)
{
    switch (slot)
    {
        case EQUIPMENT_SLOT_HEAD:      return "Head";
        case EQUIPMENT_SLOT_NECK:      return "Neck";
        case EQUIPMENT_SLOT_SHOULDERS: return "Shoulders";
        case EQUIPMENT_SLOT_BODY:      return "Shirt";
        case EQUIPMENT_SLOT_CHEST:     return "Chest";
        case EQUIPMENT_SLOT_WAIST:     return "Waist";
        case EQUIPMENT_SLOT_LEGS:      return "Legs";
        case EQUIPMENT_SLOT_FEET:      return "Feet";
        case EQUIPMENT_SLOT_WRISTS:    return "Wrists";
        case EQUIPMENT_SLOT_HANDS:     return "Hands";
        case EQUIPMENT_SLOT_FINGER1:   return "Ring1";
        case EQUIPMENT_SLOT_FINGER2:   return "Ring2";
        case EQUIPMENT_SLOT_TRINKET1:  return "Trink1";
        case EQUIPMENT_SLOT_TRINKET2:  return "Trink2";
        case EQUIPMENT_SLOT_BACK:      return "Back";
        case EQUIPMENT_SLOT_MAINHAND:  return "MH";
        case EQUIPMENT_SLOT_OFFHAND:   return "OH";
        case EQUIPMENT_SLOT_RANGED:    return "Ranged";
        case EQUIPMENT_SLOT_TABARD:    return "Tabard";
        default:                       return "E";
    }
}

// Short hex tag from item GUID for disambiguating duplicate item names in gossip
// Dynamic gossip text helper (NPC_TEXT_UPDATE packet)
static void SendDynamicGossipText(Player* player, std::string const& text, uint32 textId)
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

// =============================================================================
// Level Scaling — DBC values are the level-80 cap. Speed/Fly do NOT scale.
// =============================================================================
static uint32 ScaleEnchantAmount(uint32 baseAmount, uint8 playerLevel)
{
    if (playerLevel >= 80 || baseAmount <= 1)
        return baseAmount;

    float scaled = float(baseAmount) * float(playerLevel) / 80.0f;
    uint32 result = static_cast<uint32>(std::round(scaled));
    return std::max(result, 1u);
}

// =============================================================================
// Stat Application — Mirrors Player::ApplyEnchantment for STAT and RESISTANCE types.
// Speed/Fly enchants are handled separately via Player setter fields + UpdateSpeed.
// =============================================================================
static void ApplyLotteryEnchantStat(Player* player, uint32 enchantId, bool apply)
{
    // Custom speed/fly enchants are NOT applied here — they use the Player fields
    // and get recalculated in RecalcLotterySpeedAndFly after all items are processed.
    if (IsCustomEnchant(enchantId))
        return;

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
    if (!pEnchant)
        return;

    uint8 playerLevel = player->GetLevel();

    for (int s = 0; s < MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++s)
    {
        uint32 enchantType   = pEnchant->type[s];
        uint32 baseAmount    = pEnchant->amount[s];
        uint32 enchantStatId = pEnchant->spellid[s];

        if (baseAmount == 0)
            continue;

        // Resistance enchants: school in spellid
        if (enchantType == ITEM_ENCHANTMENT_TYPE_RESISTANCE)
        {
            uint32 enchantAmount = ScaleEnchantAmount(baseAmount, playerLevel);
            player->HandleStatFlatModifier(
                UnitMods(UNIT_MOD_RESISTANCE_START + enchantStatId),
                TOTAL_VALUE, float(enchantAmount), apply);
            continue;
        }

        if (enchantType != ITEM_ENCHANTMENT_TYPE_STAT)
            continue;

        uint32 enchantAmount = ScaleEnchantAmount(baseAmount, playerLevel);

        switch (enchantStatId)
        {
            case ITEM_MOD_MANA:
                player->HandleStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, float(enchantAmount), apply);
                break;
            case ITEM_MOD_HEALTH:
                player->HandleStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(enchantAmount), apply);
                break;
            case ITEM_MOD_AGILITY:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_VALUE, float(enchantAmount), apply);
                player->UpdateStatBuffMod(STAT_AGILITY);
                break;
            case ITEM_MOD_STRENGTH:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_VALUE, float(enchantAmount), apply);
                player->UpdateStatBuffMod(STAT_STRENGTH);
                break;
            case ITEM_MOD_INTELLECT:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, float(enchantAmount), apply);
                player->UpdateStatBuffMod(STAT_INTELLECT);
                break;
            case ITEM_MOD_SPIRIT:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, float(enchantAmount), apply);
                player->UpdateStatBuffMod(STAT_SPIRIT);
                break;
            case ITEM_MOD_STAMINA:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_VALUE, float(enchantAmount), apply);
                player->UpdateStatBuffMod(STAT_STAMINA);
                break;
            case ITEM_MOD_DEFENSE_SKILL_RATING: player->ApplyRatingMod(CR_DEFENSE_SKILL, enchantAmount, apply); break;
            case ITEM_MOD_DODGE_RATING:         player->ApplyRatingMod(CR_DODGE, enchantAmount, apply); break;
            case ITEM_MOD_PARRY_RATING:         player->ApplyRatingMod(CR_PARRY, enchantAmount, apply); break;
            case ITEM_MOD_BLOCK_RATING:         player->ApplyRatingMod(CR_BLOCK, enchantAmount, apply); break;
            case ITEM_MOD_HIT_MELEE_RATING:     player->ApplyRatingMod(CR_HIT_MELEE, enchantAmount, apply); break;
            case ITEM_MOD_HIT_RANGED_RATING:    player->ApplyRatingMod(CR_HIT_RANGED, enchantAmount, apply); break;
            case ITEM_MOD_HIT_SPELL_RATING:     player->ApplyRatingMod(CR_HIT_SPELL, enchantAmount, apply); break;
            case ITEM_MOD_CRIT_MELEE_RATING:    player->ApplyRatingMod(CR_CRIT_MELEE, enchantAmount, apply); break;
            case ITEM_MOD_CRIT_RANGED_RATING:   player->ApplyRatingMod(CR_CRIT_RANGED, enchantAmount, apply); break;
            case ITEM_MOD_CRIT_SPELL_RATING:    player->ApplyRatingMod(CR_CRIT_SPELL, enchantAmount, apply); break;
            case ITEM_MOD_HASTE_RANGED_RATING:  player->ApplyRatingMod(CR_HASTE_RANGED, enchantAmount, apply); break;
            case ITEM_MOD_HASTE_SPELL_RATING:   player->ApplyRatingMod(CR_HASTE_SPELL, enchantAmount, apply); break;
            case ITEM_MOD_HIT_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, enchantAmount, apply);
                player->ApplyRatingMod(CR_HIT_RANGED, enchantAmount, apply);
                player->ApplyRatingMod(CR_HIT_SPELL, enchantAmount, apply);
                break;
            case ITEM_MOD_CRIT_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, enchantAmount, apply);
                player->ApplyRatingMod(CR_CRIT_RANGED, enchantAmount, apply);
                player->ApplyRatingMod(CR_CRIT_SPELL, enchantAmount, apply);
                break;
            case ITEM_MOD_RESILIENCE_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchantAmount, apply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchantAmount, apply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchantAmount, apply);
                break;
            case ITEM_MOD_HASTE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, enchantAmount, apply);
                player->ApplyRatingMod(CR_HASTE_RANGED, enchantAmount, apply);
                player->ApplyRatingMod(CR_HASTE_SPELL, enchantAmount, apply);
                break;
            case ITEM_MOD_EXPERTISE_RATING:     player->ApplyRatingMod(CR_EXPERTISE, enchantAmount, apply); break;
            case ITEM_MOD_ATTACK_POWER:
                player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(enchantAmount), apply);
                player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(enchantAmount), apply);
                break;
            case ITEM_MOD_RANGED_ATTACK_POWER:
                player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(enchantAmount), apply);
                break;
            case ITEM_MOD_MANA_REGENERATION:     player->ApplyManaRegenBonus(enchantAmount, apply); break;
            case ITEM_MOD_ARMOR_PENETRATION_RATING: player->ApplyRatingMod(CR_ARMOR_PENETRATION, enchantAmount, apply); break;
            case ITEM_MOD_SPELL_POWER:           player->ApplySpellPowerBonus(enchantAmount, apply); break;
            case ITEM_MOD_HEALTH_REGEN:          player->ApplyHealthRegenBonus(enchantAmount, apply); break;
            case ITEM_MOD_SPELL_PENETRATION:     player->ApplySpellPenetrationBonus(enchantAmount, apply); break;
            case ITEM_MOD_BLOCK_VALUE:
                player->HandleBaseModFlatValue(SHIELD_BLOCK_VALUE, float(enchantAmount), apply);
                break;
            default: break;
        }
    }
}

// Apply/unapply ALL lottery enchants for a given item on a player
static void ApplyAllLotteryEnchantsForItem(Player* player, uint32 itemGuid, bool apply)
{
    std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
    auto it = s_lotteryCache.find(itemGuid);
    if (it == s_lotteryCache.end())
        return;

    for (uint32 enchantId : it->second)
        ApplyLotteryEnchantStat(player, enchantId, apply);
}

// =============================================================================
// Speed/Fly Recalculation — scans ALL equipped items, updates Player fields,
// triggers UpdateSpeed and SetCanFly/double-jump. Called on equip/unequip/login.
//
// MOBILITY BOOST MODES:
//   Full Flight:  level >= 60 AND not flylock → sustained flying, flight speed
//   Double Jump:  level < 60 OR flylock → one upward boost per airborne session
// =============================================================================
static void RecalcLotterySpeedAndFly(Player* player)
{
    float totalSpeedBonus = 0.0f;
    uint32 highestFlyStage = 0;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item) continue;

        uint32 guid = item->GetGUID().GetCounter();
        std::vector<uint32> enchants;
        {
            std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
            auto it = s_lotteryCache.find(guid);
            if (it != s_lotteryCache.end())
                enchants = it->second;
        }

        for (uint32 enchantId : enchants)
        {
            if (IsCustomSpeedEnchant(enchantId))
            {
                uint32 rawPct = GetSpeedPct(enchantId);
                totalSpeedBonus += float(rawPct) / 100.0f;
            }
            else if (IsCustomFlyEnchant(enchantId))
            {
                uint32 stage = GetFlyStage(enchantId);
                if (stage > highestFlyStage)
                    highestFlyStage = stage;
            }
        }
    }

    totalSpeedBonus = std::min(totalSpeedBonus, 1.0f);
    float flyRate = float(highestFlyStage);

    player->SetLotterySpeedBonus(totalSpeedBonus);
    player->SetLotteryFlySpeedRate(flyRate);
    player->SetLotteryCanFly(highestFlyStage > 0);

    // Force speed recalculation (includes our lottery bonus via UpdateSpeed patch)
    player->UpdateSpeed(MOVE_RUN, true);
    player->UpdateSpeed(MOVE_SWIM, true);

    bool hasFlyEnchant = player->GetLotteryCanFly();
    bool fullFlight    = player->HasLotteryFullFlight();
    bool doubleJump    = hasFlyEnchant && !fullFlight;
    uint32 pg          = player->GetGUID().GetCounter();

    if (fullFlight)
    {
        // Full flight mode: sustained flying with flight speed calculations.
        player->UpdateSpeed(MOVE_FLIGHT, true);
        player->SetCanFly(true);

        // BG flag carrier enforcement
        if (player->InBattleground() && player->IsFlying())
        {
            if (player->HasAura(23335) || player->HasAura(23333) || player->HasAura(34976))
            {
                player->RemoveAurasDueToSpell(23335);
                player->RemoveAurasDueToSpell(23333);
                player->RemoveAurasDueToSpell(34976);
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "|cffFF0000You can't carry a flag while flying! Flag dropped.|r");
            }
        }
    }
    else if (doubleJump)
    {
        // Double jump mode: SetCanFly enables the space-bar trigger while airborne.
        // OnPlayerUpdate detects IsFlying() → applies upward knockback → removes fly.
        // If double jump was already used this airborne session, leave CAN_FLY off
        // until the player lands (OnPlayerUpdate handles re-enabling).
        if (!s_doubleJumpUsed[pg])
            player->SetCanFly(true);

        // Transition from full-fly to double-jump while airborne: the player
        // is currently flying and toggled flylock or leveled down (hypothetical).
        // They keep flying this one time; OnPlayerUpdate will handle landing transition.
    }
    else
    {
        // No fly enchant — disable flight entirely.
        s_doubleJumpUsed.erase(pg);
        if (player->IsFlying())
        {
            player->SetCanFly(false);
            player->CastSpell(player, 130, true); // Slow Fall to prevent splat
        }
        else
        {
            player->SetCanFly(false);
        }
    }
}

// =============================================================================
// Full Recalc — swap-safe stat application.
//
// Item swaps (right-click to equip from inventory) fire OnPlayerEquip for the
// new item but do NOT fire OnPlayerUnequip for the displaced item. Incremental
// apply/unapply would leave the old item's bonuses orphaned → infinite stacking.
//
// This function does a FULL recalc: unapply everything tracked, then reapply
// only for items that are CURRENTLY equipped. Called on every equip, unequip,
// and login. Cost: ~19 slots × 7 enchants = 133 stat ops max — trivial.
// =============================================================================
static void FullRecalcLotteryStats(Player* player)
{
    uint32 pg = player->GetGUID().GetCounter();
    auto& applied = s_appliedItems[pg];

    // Step 1: Unapply all currently tracked items
    for (uint32 itemGuid : applied)
        ApplyAllLotteryEnchantsForItem(player, itemGuid, false);
    applied.clear();

    // Step 2: Apply enchants for items that are CURRENTLY equipped
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item) continue;

        uint32 itemGuid = item->GetGUID().GetCounter();
        {
            std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
            if (s_lotteryCache.find(itemGuid) == s_lotteryCache.end())
                continue;
        }
        ApplyAllLotteryEnchantsForItem(player, itemGuid, true);
        applied.insert(itemGuid);
    }

    // Step 3: Recalculate speed and fly from all equipped items
    RecalcLotterySpeedAndFly(player);
}

// =============================================================================
// Level-up recalculation: delta-based stat adjustment for equipped items.
// Speed/Fly enchants are now also level-scaled, so they need recalculation too.
// =============================================================================
static void RefreshLotteryEnchantsOnLevelUp(Player* player, uint8 /*oldLevel*/)
{
    // Full recalc: unapply all at old-level values, reapply at new-level values.
    // Handles DBC enchant scaling AND speed enchant scaling in one pass.
    // Cost is negligible for an event that fires 80 times in a character's lifetime.
    FullRecalcLotteryStats(player);
}

// =============================================================================
// Build enchant pools from SpellItemEnchantment.dbc at startup.
// Accepts PURE single-effect enchantments of type STAT or RESISTANCE.
// Dodge/Parry/Defense excluded. Physical resist excluded.
// =============================================================================
static void BuildEnchantPoolsFromDBC()
{
    if (s_poolsBuilt) return;

    uint32 totalStats = 0, totalResist = 0, totalSkipped = 0;

    for (uint32 id = 1; id < sSpellItemEnchantmentStore.GetNumRows(); ++id)
    {
        SpellItemEnchantmentEntry const* entry = sSpellItemEnchantmentStore.LookupEntry(id);
        if (!entry) continue;
        if (entry->GemID || entry->EnchantmentCondition || entry->requiredSkill) continue;

        int effectIndex = -1;
        bool valid = true;

        for (uint32 e = 0; e < MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++e)
        {
            if (entry->type[e] == ITEM_ENCHANTMENT_TYPE_NONE) continue;
            if (entry->type[e] == ITEM_ENCHANTMENT_TYPE_STAT || entry->type[e] == ITEM_ENCHANTMENT_TYPE_RESISTANCE)
            {
                if (effectIndex >= 0) { valid = false; break; }
                effectIndex = e;
            }
            else { valid = false; break; }
        }

        if (!valid || effectIndex < 0) { ++totalSkipped; continue; }
        uint32 effectValue = entry->amount[effectIndex];
        if (effectValue == 0) { ++totalSkipped; continue; }

        if (entry->type[effectIndex] == ITEM_ENCHANTMENT_TYPE_STAT)
        {
            uint32 statType = entry->spellid[effectIndex];
            if (!IsAllowedStatType(statType)) { ++totalSkipped; continue; }
            if (effectValue > s_globalMaxEnchantValue) s_globalMaxEnchantValue = effectValue;
            s_enchantPools[statType].push_back(id);
            ++totalStats;
        }
        else
        {
            uint32 school = entry->spellid[effectIndex];
            if (!IsAllowedResistSchool(school)) { ++totalSkipped; continue; }
            if (effectValue > s_globalMaxEnchantValue) s_globalMaxEnchantValue = effectValue;
            s_enchantPools[RESIST_POOL_KEY_BASE + school].push_back(id);
            ++totalResist;
        }
    }

    s_poolsBuilt = true;
    LOG_INFO("module", "LotteryEnchants: Built pools — {} stat + {} resist enchants across {} types + Movespeed + Flying, max value {}, {} skipped",
             totalStats, totalResist, s_enchantPools.size(), s_globalMaxEnchantValue, totalSkipped);
}

// =============================================================================
// Get a random enchant — POOL-TYPE-FIRST approach for equal type weighting.
// 1% chance → Flying enchant (sub-roll 75/20/5% for stage 1/2/3).
// 99% chance → pick a random pool type (including Movespeed), then a random
// enchant from that type. ALL enchant values available for ALL item qualities —
// the percentile system handles balance (low rolls are common, god-rolls rare).
// =============================================================================
static uint32 GetRandomEnchant(Item* item, const std::vector<uint32>& excludeSet)
{
    if (!item) return 0;

    // --- 1% chance: Flying enchant ---
    if (urand(1, 100) == 1)
    {
        uint32 flyRoll = urand(1, 100);
        uint32 flyId;
        if (flyRoll <= 75)      flyId = CUSTOM_ENCHANT_FLY_BASE + 1; // Stage 1 (100%)
        else if (flyRoll <= 95) flyId = CUSTOM_ENCHANT_FLY_BASE + 2; // Stage 2 (200%)
        else                    flyId = CUSTOM_ENCHANT_FLY_BASE + 3; // Stage 3 (300%)

        // Allow even if same stage exists in exclude (different items can have same stage)
        // Only skip if the EXACT same ID is excluded on THIS item (prevents duplicate stage)
        if (std::find(excludeSet.begin(), excludeSet.end(), flyId) == excludeSet.end())
            return flyId;
        // Fall through to normal pool if excluded
    }

    // --- Build available pool type keys (stat types + resist schools + Speed) ---
    std::vector<uint32> poolKeys;
    poolKeys.reserve(s_enchantPools.size() + 1);

    for (auto& [key, ids] : s_enchantPools)
    {
        // Check if this pool type has at least one valid non-excluded enchant
        for (uint32 id : ids)
        {
            if (std::find(excludeSet.begin(), excludeSet.end(), id) != excludeSet.end())
                continue;
            poolKeys.push_back(key);
            goto nextPool;
        }
        nextPool:;
    }

    // Always add Speed as a pool type (no quality restriction)
    poolKeys.push_back(SPEED_POOL_KEY);

    if (poolKeys.empty())
        return 0;

    // --- Pick a random pool type (equal chance for each type) ---
    // Retry up to 10 times if the selected enchant happens to be excluded
    for (uint32 attempt = 0; attempt < 10; ++attempt)
    {
        uint32 chosenKey = poolKeys[urand(0, poolKeys.size() - 1)];

        if (chosenKey == SPEED_POOL_KEY)
        {
            // Movespeed: random 1-25%
            uint32 speedPct = urand(1, 25);
            uint32 speedId = CUSTOM_ENCHANT_SPEED_BASE + speedPct;
            if (std::find(excludeSet.begin(), excludeSet.end(), speedId) == excludeSet.end())
                return speedId;
            continue; // Exact same percentage excluded, retry
        }

        // Regular DBC enchant from the chosen pool type
        auto& enchantIds = s_enchantPools[chosenKey];
        std::vector<uint32> valid;
        valid.reserve(enchantIds.size());

        for (uint32 id : enchantIds)
        {
            if (std::find(excludeSet.begin(), excludeSet.end(), id) != excludeSet.end())
                continue;
            valid.push_back(id);
        }

        if (!valid.empty())
            return valid[urand(0, valid.size() - 1)];
    }

    return 0;
}

// =============================================================================
// Roll new enchants for an item — returns the vector of enchant IDs without
// applying them. Separated from application to support the Keep/Take preview.
// =============================================================================
static std::vector<uint32> RollNewEnchants(Item* item, bool isReroll)
{
    if (!item) return {};

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto) return {};
    if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR) return {};
    // All qualities (grey through legendary) are eligible — quality gates the TIER of
    // enchant that can roll (grey item = only grey-tier stats). Vendor purchase exclusion
    // is handled at the hook level, not here.
    if (proto->Quality > ITEM_QUALITY_LEGENDARY) return {};

    uint32 maxSlots = sConfigMgr->GetOption<uint32>("CustomRamvaris.LotteryEnchants.MaxSlots", MAX_LOTTERY_SLOTS);
    if (maxSlots > MAX_LOTTERY_SLOTS) maxSlots = MAX_LOTTERY_SLOTS;

    std::vector<uint32> result;
    for (uint32 rollIdx = 0; rollIdx < maxSlots; ++rollIdx)
    {
        float chance = (isReroll && rollIdx == 0) ? 100.0f : ROLL_CHANCES[rollIdx];
        if (rand_chance() >= chance) break;

        uint32 enc = GetRandomEnchant(item, result);
        if (enc == 0) break;

        result.push_back(enc);
    }

    return result;
}

// =============================================================================
// Apply rolled enchants to an item — writes DB + cache, sets 777 durability,
// notifies player. Handles wiping old enchants on reroll.
// =============================================================================
static void ApplyRolledEnchants(Player* player, Item* item, const std::vector<uint32>& enchants, bool isReroll)
{
    if (!player || !item || enchants.empty()) return;

    uint32 itemGuid = item->GetGUID().GetCounter();

    if (isReroll)
    {
        if (item->IsEquipped())
            ApplyAllLotteryEnchantsForItem(player, itemGuid, false);

        {
            std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
            s_lotteryCache.erase(itemGuid);
        }
        auto* delStmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_LOTTERY_ENCHANTS);
        delStmt->SetData(0, itemGuid);
        CharacterDatabase.Execute(delStmt);
    }

    // Write to DB
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    for (uint32 i = 0; i < enchants.size(); ++i)
    {
        auto* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEM_LOTTERY_ENCHANT);
        stmt->SetData(0, itemGuid);
        stmt->SetData(1, uint8(i));
        stmt->SetData(2, enchants[i]);
        trans->Append(stmt);
    }
    CharacterDatabase.CommitTransaction(trans);

    // Update cache
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        s_lotteryCache[itemGuid] = enchants;
    }

    // Apply stats if equipped
    if (item->IsEquipped())
    {
        for (uint32 enchantId : enchants)
            ApplyLotteryEnchantStat(player, enchantId, true);
        RecalcLotterySpeedAndFly(player);
    }

    // Visual marker: 777/777 durability
    item->SetUInt32Value(ITEM_FIELD_MAXDURABILITY, 777);
    item->SetUInt32Value(ITEM_FIELD_DURABILITY, 777);
    item->SetState(ITEM_CHANGED, player);

    // Notification
    ItemTemplate const* proto = item->GetTemplate();
    std::string itemName = GetItemDisplayName(player, proto);
    uint32 count = enchants.size();
    const char* verb = isReroll ? "rerolled" : "received";
    ChatHandler(player->GetSession()).PSendSysMessage(
        "{}{}|r {} |cff00FF00{}|r lottery enchant{}! Use |cffFFFF00.enchants|r to view.",
        GetCountColor(count), itemName, verb, count, count == 1 ? "" : "s");
}

// =============================================================================
// RollLotteryEnchants — convenience wrapper (rolls + applies in one call).
// Used by the automatic on-acquire hook. Keep/Take flow uses Roll + Apply separately.
// =============================================================================
static void RollLotteryEnchants(Player* player, Item* item, bool isReroll = false)
{
    if (!player || !item) return;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto) return;
    if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR) return;

    uint32 itemGuid = item->GetGUID().GetCounter();

    // Don't re-roll items that already have enchants (unless explicit reroll)
    if (!isReroll)
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        if (s_lotteryCache.count(itemGuid)) return;
    }

    std::vector<uint32> rolled = RollNewEnchants(item, isReroll);
    if (rolled.empty()) return;

    ApplyRolledEnchants(player, item, rolled, isReroll);
}

// =============================================================================
// Load/Unload/Delete — Login, logout, item destruction
// =============================================================================

static void LoadLotteryEnchantsForPlayer(Player* player)
{
    if (!player) return;

    // Clear any stale cache entries for this player's items BEFORE loading.
    // Without this, re-login (or any double-call) APPENDS DB rows onto existing
    // cache entries, causing items to exceed MAX_LOTTERY_SLOTS (e.g., 10 enchants
    // on an item that should cap at 7).
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                s_lotteryCache.erase(item->GetGUID().GetCounter());
        }
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                s_lotteryCache.erase(item->GetGUID().GetCounter());
        }
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            Bag* pBag = player->GetBagByPos(bag);
            if (!pBag) continue;
            for (uint32 s = 0; s < pBag->GetBagSize(); ++s)
            {
                if (Item* item = pBag->GetItemByPos(s))
                    s_lotteryCache.erase(item->GetGUID().GetCounter());
            }
        }
    }

    auto* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_LOTTERY_ENCHANTS_BY_OWNER);
    stmt->SetData(0, player->GetGUID().GetCounter());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result) return;

    uint32 loadedEnchants = 0;
    uint32 maxSlots = sConfigMgr->GetOption<uint32>("CustomRamvaris.LotteryEnchants.MaxSlots", MAX_LOTTERY_SLOTS);
    if (maxSlots > MAX_LOTTERY_SLOTS) maxSlots = MAX_LOTTERY_SLOTS;

    do
    {
        Field* fields = result->Fetch();
        uint32 itemGuid  = fields[0].Get<uint32>();
        uint32 enchantId = fields[2].Get<uint32>();

        // Validate: DBC enchant or custom enchant
        if (!IsCustomEnchant(enchantId) && !sSpellItemEnchantmentStore.LookupEntry(enchantId))
            continue;

        {
            std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
            auto& vec = s_lotteryCache[itemGuid];
            // Hard cap: refuse to load more than MAX_LOTTERY_SLOTS per item.
            // DB corruption or legacy data with >7 rows gets silently capped.
            if (vec.size() >= maxSlots)
                continue;
            vec.push_back(enchantId);
        }
        ++loadedEnchants;
    }
    while (result->NextRow());

    // Apply stats via full recalc (swap-safe, initializes tracking set clean)
    FullRecalcLotteryStats(player);

    // Re-stamp 777/777 durability on items with lottery enchants.
    // Item::LoadFromDB resets ITEM_FIELD_MAXDURABILITY to the template value (0 for
    // cloaks, etc.), wiping our visual marker. We restore it here on every login.
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        auto restampDurability = [&](Item* item) {
            if (!item) return;
            uint32 guid = item->GetGUID().GetCounter();
            auto it = s_lotteryCache.find(guid);
            if (it != s_lotteryCache.end() && !it->second.empty())
            {
                item->SetUInt32Value(ITEM_FIELD_MAXDURABILITY, 777);
                item->SetUInt32Value(ITEM_FIELD_DURABILITY, 777);
                item->SetState(ITEM_CHANGED, player);
            }
        };
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            restampDurability(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            restampDurability(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            Bag* pBag = player->GetBagByPos(bag);
            if (!pBag) continue;
            for (uint32 s = 0; s < pBag->GetBagSize(); ++s)
                restampDurability(pBag->GetItemByPos(s));
        }
    }

    if (loadedEnchants > 0)
        LOG_DEBUG("module", "LotteryEnchants: Loaded {} enchants for player {} (level {})",
                  loadedEnchants, player->GetName(), player->GetLevel());
}

static void UnloadLotteryEnchantsForPlayer(Player* player)
{
    if (!player) return;

    uint32 pg = player->GetGUID().GetCounter();

    // Unapply all tracked enchants
    auto& applied = s_appliedItems[pg];
    for (uint32 itemGuid : applied)
        ApplyAllLotteryEnchantsForItem(player, itemGuid, false);
    applied.clear();
    s_appliedItems.erase(pg);

    // Purge cache entries for this player's items to prevent stale accumulation.
    // On next login, LoadLotteryEnchantsForPlayer rebuilds from DB.
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                s_lotteryCache.erase(item->GetGUID().GetCounter());
        }
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                s_lotteryCache.erase(item->GetGUID().GetCounter());
        }
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            Bag* pBag = player->GetBagByPos(bag);
            if (!pBag) continue;
            for (uint32 s = 0; s < pBag->GetBagSize(); ++s)
            {
                if (Item* item = pBag->GetItemByPos(s))
                    s_lotteryCache.erase(item->GetGUID().GetCounter());
            }
        }
    }

    // Reset Player fields (speed/fly)
    player->SetLotterySpeedBonus(0.0f);
    player->SetLotteryFlySpeedRate(0.0f);
    player->SetLotteryCanFly(false);
    player->SetCanFly(false);

    // Clean up gossip state + double jump tracking
    s_pendingRerolls.erase(pg);
    s_rerollPage.erase(pg);
    s_enchantsPage.erase(pg);
    s_enchantsItemList.erase(pg);
    s_rerollItemList.erase(pg);
    s_doubleJumpUsed.erase(pg);
}

// Cleanup helper — deletes lottery enchants for a destroyed item from DB + cache.
// Called by the orphan cleanup on server startup, and available for future item
// destruction hooks if AzerothCore adds PLAYERHOOK_ON_ITEM_DESTROY.
static void DeleteLotteryEnchantsForItem(uint32 itemGuid) __attribute__((unused));
static void DeleteLotteryEnchantsForItem(uint32 itemGuid)
{
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        s_lotteryCache.erase(itemGuid);
    }
    auto* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_LOTTERY_ENCHANTS);
    stmt->SetData(0, itemGuid);
    CharacterDatabase.Execute(stmt);
}

// =============================================================================
// Enchant display helper — builds a text description of an enchant for gossip/chat
// =============================================================================
static std::string FormatEnchantLine(uint32 enchantId, uint8 playerLevel)
{
    if (IsCustomSpeedEnchant(enchantId))
    {
        uint32 rawPct = GetSpeedPct(enchantId);
        return std::string("|cff00FFFF+") + std::to_string(rawPct) + "% Movespeed|r";
    }
    if (IsCustomFlyEnchant(enchantId))
    {
        uint32 stage = GetFlyStage(enchantId);
        uint32 flyPct = stage * 100;
        return std::string("|cffFF00FF+Mobility Boost Stage ") + std::to_string(stage) + " (" + std::to_string(flyPct) + "% flight speed)|r";
    }

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
    if (!pEnchant) return "|cff888888Unknown Enchant|r";

    for (int s = 0; s < MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++s)
    {
        if (pEnchant->amount[s] == 0) continue;

        uint32 baseAmount = pEnchant->amount[s];
        uint32 scaledAmount = ScaleEnchantAmount(baseAmount, playerLevel);
        EnchantTier valueTier = GetTierFromValue(baseAmount);
        const char* name = "Unknown";

        if (pEnchant->type[s] == ITEM_ENCHANTMENT_TYPE_STAT)
            name = GetStatName(pEnchant->spellid[s]);
        else if (pEnchant->type[s] == ITEM_ENCHANTMENT_TYPE_RESISTANCE)
            name = GetResistName(pEnchant->spellid[s]);
        else
            continue;

        std::string line;
        line += GetValueColor(valueTier);
        line += "+";
        line += std::to_string(scaledAmount);
        line += " ";
        line += name;
        line += "|r";

        if (playerLevel < 80)
        {
            line += " |cff888888(max: ";
            line += std::to_string(baseAmount);
            line += ")|r";
        }
        return line;
    }
    return "|cff888888Unknown|r";
}

// =============================================================================
// Gossip Menu Helpers — Reroll flow and Enchants viewer
// =============================================================================

// --- Build eligible bag items list for reroll menu ---
static void BuildRerollItemList(Player* player)
{
    uint32 pg = player->GetGUID().GetCounter();
    auto& items = s_rerollItemList[pg];
    items.clear();

    auto addItem = [&](Item* item)
    {
        if (!item) return;
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto) return;
        if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR) return;

        uint32 guid = item->GetGUID().GetCounter();
        uint32 enchantCount = 0;
        {
            std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
            auto it = s_lotteryCache.find(guid);
            if (it != s_lotteryCache.end())
                enchantCount = it->second.size();
        }
        items.push_back({guid, proto->ItemId, enchantCount});
    };

    // Main backpack only — no equipped items, no bank
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

    // Extra bags (1-4)
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = player->GetBagByPos(bag);
        if (!pBag) continue;
        for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            addItem(pBag->GetItemByPos(slot));
    }
}

// --- Show paginated reroll item list ---
static void ShowRerollMenu(Player* player, uint32 page)
{
    uint32 pg = player->GetGUID().GetCounter();
    BuildRerollItemList(player);
    s_rerollPage[pg] = page;

    // Clean any stale pending reroll
    s_pendingRerolls.erase(pg);

    auto& items = s_rerollItemList[pg];
    uint32 totalItems = items.size();
    uint32 totalPages = (totalItems + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (totalPages == 0) totalPages = 1;
    if (page >= totalPages) page = totalPages - 1;
    s_rerollPage[pg] = page;

    ClearGossipMenuFor(player);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(LOTTERY_GOSSIP_MENU_ID);

    if (items.empty())
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            "|cff888888No eligible items in your bags.|r",
            LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);
    }
    else
    {
        // Navigation at top
        if (totalPages > 1 && page < totalPages - 1)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                "|TInterface\\Icons\\Ability_Druid_Dash:20:20:-2:0|t Next Page >>",
                LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_NEXT_PAGE);
        if (totalPages > 1 && page > 0)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                "|TInterface\\Icons\\Ability_Druid_Dash_Orange:20:20:-2:0|t << Previous Page",
                LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_PREV_PAGE);

        if (totalPages > 1)
        {
            std::string pageInfo = "|cff666666[ Page " + std::to_string(page + 1) + " / " +
                                   std::to_string(totalPages) + " — " + std::to_string(totalItems) +
                                   " items — " + std::to_string(REROLL_COST_GOLD) + "g per roll ]|r";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, pageInfo, LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);
        }

        // Items on this page
        uint32 startIdx = page * ITEMS_PER_PAGE;
        uint32 endIdx = std::min(startIdx + ITEMS_PER_PAGE, totalItems);

        for (uint32 i = startIdx; i < endIdx; ++i)
        {
            auto& ri = items[i];
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(ri.itemEntry);
            if (!proto) continue;

            std::string name = GetItemDisplayName(player, proto);
            std::string label;
            if (ri.enchantCount > 0)
                label = std::string(GetCountColor(ri.enchantCount)) + name + "|r |cff888888(" +
                        std::to_string(ri.enchantCount) + " enchants)|r";
            else
                label = "|cffBBBBBB" + name + "|r |cff888888(no enchants)|r";

            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, label,
                LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_REROLL_BASE + (i - startIdx));
        }
    }

    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        "|TInterface\\Icons\\Misc_ArrowLeft:20:20:-2:0|t Close",
        LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);

    SendDynamicGossipText(player, "Select an item to reroll for " + std::to_string(REROLL_COST_GOLD) +
                          " gold.\nFirst enchant is guaranteed. Gold is spent on roll.", LOTTERY_NPC_TEXT_ID);
    SendGossipMenuFor(player, LOTTERY_NPC_TEXT_ID, player->GetGUID());
}

// --- Show reroll confirmation for a specific item ---
static void ShowRerollConfirm(Player* player, uint32 itemGuid)
{
    uint32 pg = player->GetGUID().GetCounter();

    // Find the item in bags — must still exist
    Item* item = nullptr;
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (it && it->GetGUID().GetCounter() == itemGuid)
        { item = it; break; }
    }
    if (!item)
    {
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END && !item; ++bag)
        {
            Bag* pBag = player->GetBagByPos(bag);
            if (!pBag) continue;
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* it = pBag->GetItemByPos(slot);
                if (it && it->GetGUID().GetCounter() == itemGuid)
                { item = it; break; }
            }
        }
    }

    if (!item)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Item no longer in bags.|r");
        CloseGossipMenuFor(player);
        return;
    }

    ItemTemplate const* proto = item->GetTemplate();
    std::string name = GetItemDisplayName(player, proto);

    // Store the item GUID for the confirm action
    s_pendingRerolls[pg] = {itemGuid, {}};

    ClearGossipMenuFor(player);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(LOTTERY_GOSSIP_MENU_ID);

    std::string confirmLabel = "|cffFF0000Roll " + name + " for " + std::to_string(REROLL_COST_GOLD) +
                               "g|r |cff888888(gold deducted now)|r";
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, confirmLabel, LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_REROLL_CONFIRM);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        "|TInterface\\Icons\\Misc_ArrowLeft:20:20:-2:0|t Back",
        LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);

    SendDynamicGossipText(player, "Are you sure you want to reroll " + name + "?\n\n"
        "Cost: " + std::to_string(REROLL_COST_GOLD) + " gold (deducted NOW on roll).\n"
        "You will preview the new enchants before deciding to TAKE or KEEP old ones.",
        LOTTERY_NPC_TEXT_ID);
    SendGossipMenuFor(player, LOTTERY_NPC_TEXT_ID, player->GetGUID());
}

// --- Show reroll preview with Take/Keep options ---
static void ShowRerollPreview(Player* player)
{
    uint32 pg = player->GetGUID().GetCounter();
    auto pendIt = s_pendingRerolls.find(pg);
    if (pendIt == s_pendingRerolls.end())
    {
        CloseGossipMenuFor(player);
        return;
    }

    auto& pending = pendIt->second;
    if (pending.rolledEnchants.empty())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Roll failed — no enchants generated.|r");
        s_pendingRerolls.erase(pg);
        CloseGossipMenuFor(player);
        return;
    }

    ClearGossipMenuFor(player);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(LOTTERY_GOSSIP_MENU_ID);

    // Show rolled enchants as gossip items (informational)
    uint8 plvl = player->GetLevel();
    for (uint32 i = 0; i < pending.rolledEnchants.size(); ++i)
    {
        std::string line = "  " + FormatEnchantLine(pending.rolledEnchants[i], plvl);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, line, LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);
    }

    // Take and Keep buttons
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE,
        "|cff00FF00>> TAKE (Apply these enchants) <<|r",
        LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_REROLL_TAKE);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        "|cffFF8800>> KEEP (Keep old enchants, discard roll) <<|r",
        LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_REROLL_KEEP);

    std::string countStr = std::to_string(pending.rolledEnchants.size());
    SendDynamicGossipText(player,
        "Rolled " + countStr + " new enchant" + (pending.rolledEnchants.size() > 1 ? "s" : "") +
        "!\n\nTAKE = Apply new enchants (old ones are DESTROYED).\n"
        "KEEP = Discard this roll, keep old enchants.\n"
        "Gold has already been deducted.", LOTTERY_NPC_TEXT_ID);
    SendGossipMenuFor(player, LOTTERY_NPC_TEXT_ID, player->GetGUID());
}

// --- Build enchant viewer item list ---
static void BuildEnchantsItemList(Player* player)
{
    uint32 pg = player->GetGUID().GetCounter();
    auto& items = s_enchantsItemList[pg];
    items.clear();

    auto addItem = [&](Item* item, bool equipped, uint8 eqSlot = 0xFF)
    {
        if (!item) return;
        uint32 guid = item->GetGUID().GetCounter();
        uint32 enchantCount = 0;
        {
            std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
            auto it = s_lotteryCache.find(guid);
            if (it != s_lotteryCache.end())
                enchantCount = it->second.size();
        }
        if (enchantCount == 0) return;
        items.push_back({guid, item->GetTemplate()->ItemId, enchantCount, equipped, eqSlot});
    };

    // Equipped items first
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot), true, slot);

    // Main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot), false);

    // Extra bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = player->GetBagByPos(bag);
        if (!pBag) continue;
        for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            addItem(pBag->GetItemByPos(slot), false);
    }
}

// --- Show paginated enchants viewer (item list) ---
static void ShowEnchantsMenu(Player* player, uint32 page)
{
    uint32 pg = player->GetGUID().GetCounter();
    BuildEnchantsItemList(player);
    s_enchantsPage[pg] = page;

    auto& items = s_enchantsItemList[pg];
    uint32 totalItems = items.size();
    uint32 totalPages = (totalItems + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (totalPages == 0) totalPages = 1;
    if (page >= totalPages) page = totalPages - 1;
    s_enchantsPage[pg] = page;

    ClearGossipMenuFor(player);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(LOTTERY_GOSSIP_MENU_ID);

    uint8 plvl = player->GetLevel();
    uint32 scalePct = uint32(float(plvl) / 80.0f * 100.0f);

    if (items.empty())
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            "|cff888888No lottery enchants found on any items.|r",
            LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);
    }
    else
    {
        // Navigation at top
        if (totalPages > 1 && page < totalPages - 1)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                "|TInterface\\Icons\\Ability_Druid_Dash:20:20:-2:0|t Next Page >>",
                LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_ENCHANTS_NEXT);
        if (totalPages > 1 && page > 0)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                "|TInterface\\Icons\\Ability_Druid_Dash_Orange:20:20:-2:0|t << Previous Page",
                LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_ENCHANTS_PREV);

        if (totalPages > 1)
        {
            std::string pageInfo = "|cff666666[ Page " + std::to_string(page + 1) + " / " +
                                   std::to_string(totalPages) + " — " + std::to_string(totalItems) + " items ]|r";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, pageInfo, LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);
        }

        uint32 startIdx = page * ITEMS_PER_PAGE;
        uint32 endIdx = std::min(startIdx + ITEMS_PER_PAGE, totalItems);

        for (uint32 i = startIdx; i < endIdx; ++i)
        {
            auto& ei = items[i];
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(ei.itemEntry);
            if (!proto) continue;

            std::string name = GetItemDisplayName(player, proto);

            // Equipped items: show slot name with green prefix.
            std::string prefix;
            if (ei.equipped)
                prefix = "|cff00FF00[" + std::string(GetEquipSlotShort(ei.equipSlot)) + "]|r ";

            std::string label = prefix + std::string(GetCountColor(ei.enchantCount)) + name +
                                "|r |cff888888(" + std::to_string(ei.enchantCount) + ")|r";

            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, label,
                LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_ENCHANTS_BASE + (i - startIdx));
        }
    }

    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        "|TInterface\\Icons\\Misc_ArrowLeft:20:20:-2:0|t Close",
        LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);

    SendDynamicGossipText(player, "Lottery Enchants — Level " + std::to_string(plvl) +
                          " (" + std::to_string(scalePct) + "% scaling)\n"
                          "Click an item to view its enchant details.",
                          LOTTERY_NPC_TEXT_ID);
    SendGossipMenuFor(player, LOTTERY_NPC_TEXT_ID, player->GetGUID());
}

// --- Show enchant details for a specific item ---
static void ShowEnchantDetails(Player* player, uint32 itemGuid)
{
    std::vector<uint32> enchants;
    {
        std::lock_guard<std::mutex> lock(s_lotteryCacheMutex);
        auto it = s_lotteryCache.find(itemGuid);
        if (it == s_lotteryCache.end())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Item no longer has enchants.|r");
            CloseGossipMenuFor(player);
            return;
        }
        enchants = it->second;
    }

    ClearGossipMenuFor(player);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(LOTTERY_GOSSIP_MENU_ID);

    uint8 plvl = player->GetLevel();
    for (uint32 enchantId : enchants)
    {
        std::string line = "  " + FormatEnchantLine(enchantId, plvl);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, line, LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_CLOSE);
    }

    // Back button returns to enchants list
    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        "|TInterface\\Icons\\Misc_ArrowLeft:20:20:-2:0|t Back to list",
        LOTTERY_GOSSIP_SENDER, LOTTO_ACTION_ENCHANTS_BACK);

    SendDynamicGossipText(player, "Enchant details (" + std::to_string(enchants.size()) + " enchants):",
                          LOTTERY_NPC_TEXT_ID);
    SendGossipMenuFor(player, LOTTERY_NPC_TEXT_ID, player->GetGUID());
}

// =============================================================================
// Gossip Select Handler — routes all lottery gossip actions
// =============================================================================
static void HandleLotteryGossipSelect(Player* player, uint32 action)
{
    uint32 pg = player->GetGUID().GetCounter();
    CloseGossipMenuFor(player);

    switch (action)
    {
        case LOTTO_ACTION_CLOSE:
            s_pendingRerolls.erase(pg);
            return;

        // === Reroll navigation ===
        case LOTTO_ACTION_NEXT_PAGE:
        {
            uint32 p = s_rerollPage.count(pg) ? s_rerollPage[pg] + 1 : 0;
            ShowRerollMenu(player, p);
            return;
        }
        case LOTTO_ACTION_PREV_PAGE:
        {
            uint32 p = s_rerollPage.count(pg) && s_rerollPage[pg] > 0 ? s_rerollPage[pg] - 1 : 0;
            ShowRerollMenu(player, p);
            return;
        }
        case LOTTO_ACTION_FIRST_PAGE:
            ShowRerollMenu(player, 0);
            return;
        case LOTTO_ACTION_LAST_PAGE:
        {
            auto& items = s_rerollItemList[pg];
            uint32 lastPage = items.empty() ? 0 : (items.size() - 1) / ITEMS_PER_PAGE;
            ShowRerollMenu(player, lastPage);
            return;
        }

        // === Reroll confirm ===
        case LOTTO_ACTION_REROLL_CONFIRM:
        {
            auto pendIt = s_pendingRerolls.find(pg);
            if (pendIt == s_pendingRerolls.end()) return;

            uint32 itemGuid = pendIt->second.itemGuid;

            // Re-find item in bags (safety: item may have been moved)
            Item* item = nullptr;
            for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            {
                Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (it && it->GetGUID().GetCounter() == itemGuid)
                { item = it; break; }
            }
            if (!item)
            {
                for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END && !item; ++bag)
                {
                    Bag* pBag = player->GetBagByPos(bag);
                    if (!pBag) continue;
                    for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
                    {
                        Item* it = pBag->GetItemByPos(slot);
                        if (it && it->GetGUID().GetCounter() == itemGuid)
                        { item = it; break; }
                    }
                }
            }

            if (!item)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Item no longer in bags.|r");
                s_pendingRerolls.erase(pg);
                return;
            }

            // Check gold
            uint32 costCopper = REROLL_COST_GOLD * 10000;
            if (player->GetMoney() < costCopper)
            {
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "|cffFF0000You need {}g. You have {}g.|r",
                    REROLL_COST_GOLD, player->GetMoney() / 10000);
                s_pendingRerolls.erase(pg);
                return;
            }

            // DEDUCT GOLD NOW (before showing results — prevents reopen abuse)
            player->ModifyMoney(-int32(costCopper));
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFD700-{}g|r — Rolling...", REROLL_COST_GOLD);

            // Roll enchants (NOT applied yet)
            std::vector<uint32> rolled = RollNewEnchants(item, true);
            if (rolled.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Roll failed — no enchants generated. Gold spent.|r");
                s_pendingRerolls.erase(pg);
                return;
            }

            pendIt->second.rolledEnchants = rolled;

            // Show preview
            ShowRerollPreview(player);
            return;
        }

        // === Take (apply rolled enchants) ===
        case LOTTO_ACTION_REROLL_TAKE:
        {
            auto pendIt = s_pendingRerolls.find(pg);
            if (pendIt == s_pendingRerolls.end()) return;

            uint32 itemGuid = pendIt->second.itemGuid;
            auto enchants = pendIt->second.rolledEnchants;
            s_pendingRerolls.erase(pg);

            // Re-find item
            Item* item = nullptr;
            for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            {
                Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (it && it->GetGUID().GetCounter() == itemGuid)
                { item = it; break; }
            }
            if (!item)
            {
                for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END && !item; ++bag)
                {
                    Bag* pBag = player->GetBagByPos(bag);
                    if (!pBag) continue;
                    for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
                    {
                        Item* it = pBag->GetItemByPos(slot);
                        if (it && it->GetGUID().GetCounter() == itemGuid)
                        { item = it; break; }
                    }
                }
            }

            if (!item)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Item gone! Enchants lost.|r");
                return;
            }

            // Apply!
            ApplyRolledEnchants(player, item, enchants, true);
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00New enchants applied!|r");
            return;
        }

        // === Keep (discard rolled enchants, keep old) ===
        case LOTTO_ACTION_REROLL_KEEP:
        {
            s_pendingRerolls.erase(pg);
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF8800Roll discarded. Old enchants kept. Gold spent.|r");
            return;
        }

        // === Enchants viewer navigation ===
        case LOTTO_ACTION_ENCHANTS_NEXT:
        {
            uint32 p = s_enchantsPage.count(pg) ? s_enchantsPage[pg] + 1 : 0;
            ShowEnchantsMenu(player, p);
            return;
        }
        case LOTTO_ACTION_ENCHANTS_PREV:
        {
            uint32 p = s_enchantsPage.count(pg) && s_enchantsPage[pg] > 0 ? s_enchantsPage[pg] - 1 : 0;
            ShowEnchantsMenu(player, p);
            return;
        }
        case LOTTO_ACTION_ENCHANTS_FIRST:
            ShowEnchantsMenu(player, 0);
            return;
        case LOTTO_ACTION_ENCHANTS_LAST:
        {
            auto& items = s_enchantsItemList[pg];
            uint32 lastPage = items.empty() ? 0 : (items.size() - 1) / ITEMS_PER_PAGE;
            ShowEnchantsMenu(player, lastPage);
            return;
        }
        case LOTTO_ACTION_ENCHANTS_BACK:
        {
            uint32 p = s_enchantsPage.count(pg) ? s_enchantsPage[pg] : 0;
            ShowEnchantsMenu(player, p);
            return;
        }

        default:
            break;
    }

    // === Reroll item selection (action = LOTTO_ACTION_REROLL_BASE + local index) ===
    if (action >= LOTTO_ACTION_REROLL_BASE && action <= LOTTO_ACTION_REROLL_MAX)
    {
        uint32 localIdx = action - LOTTO_ACTION_REROLL_BASE;
        uint32 page = s_rerollPage.count(pg) ? s_rerollPage[pg] : 0;
        uint32 globalIdx = page * ITEMS_PER_PAGE + localIdx;

        auto& items = s_rerollItemList[pg];
        if (globalIdx >= items.size())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Invalid selection — items may have changed.|r");
            return;
        }

        ShowRerollConfirm(player, items[globalIdx].itemGuid);
        return;
    }

    // === Enchants item detail (action = LOTTO_ACTION_ENCHANTS_BASE + local index) ===
    if (action >= LOTTO_ACTION_ENCHANTS_BASE && action <= LOTTO_ACTION_ENCHANTS_MAX)
    {
        uint32 localIdx = action - LOTTO_ACTION_ENCHANTS_BASE;
        uint32 page = s_enchantsPage.count(pg) ? s_enchantsPage[pg] : 0;
        uint32 globalIdx = page * ITEMS_PER_PAGE + localIdx;

        auto& items = s_enchantsItemList[pg];
        if (globalIdx >= items.size())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Item index invalid — close and reopen.|r");
            return;
        }

        ShowEnchantDetails(player, items[globalIdx].itemGuid);
        return;
    }
}

// =============================================================================
// PlayerScript: hooks for acquisition, equip, login, gossip, BG flag check
// =============================================================================
class LotteryEnchants_PlayerScript : public PlayerScript
{
public:
    LotteryEnchants_PlayerScript() : PlayerScript("LotteryEnchants_PlayerScript", {
        PLAYERHOOK_ON_STORE_NEW_ITEM,
        PLAYERHOOK_ON_BEFORE_BUY_ITEM_FROM_VENDOR,
        PLAYERHOOK_ON_EQUIP,
        PLAYERHOOK_ON_UNEQUIP_ITEM,
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_LEVEL_CHANGED,
        PLAYERHOOK_ON_GOSSIP_SELECT,
        PLAYERHOOK_ON_UPDATE,
    }) { }

    // Fires INSIDE BuyItemFromVendorSlot BEFORE StoreNewItem is called.
    // Sets a per-player flag so the StoreNewItem hook can apply quality filtering
    // for vendor purchases: only Green+ vendor items get enchants.
    void OnPlayerBeforeBuyItemFromVendor(Player* player, ObjectGuid, uint32, uint32&, uint8, uint8, uint8) override
    {
        if (!player) return;
        s_vendorBuying.insert(player->GetGUID().GetCounter());
    }

    // Universal item acquisition hook.
    // Vendor purchases: Green+ only (grey/white vendor trash = no enchants).
    // Non-vendor sources (loot, craft, quest, mail, group roll): ANY quality enchants.
    // Vendor buyback uses Player::StoreItem (not StoreNewItem) — never fires here.
    void OnPlayerStoreNewItem(Player* player, Item* item, uint32) override
    {
        if (!IsEnabled() || !player || !item) return;

        uint32 pg = player->GetGUID().GetCounter();
        if (s_vendorBuying.erase(pg))
        {
            // Vendor purchase — only roll if item is Green (Uncommon) or better
            ItemTemplate const* proto = item->GetTemplate();
            if (!proto || proto->Quality < ITEM_QUALITY_UNCOMMON)
                return;
        }

        RollLotteryEnchants(player, item);
    }

    void OnPlayerEquip(Player* player, Item* /*it*/, uint8, uint8, bool) override
    {
        if (!IsEnabled() || !player) return;
        // Full recalc on every equip — swap-safe. OnPlayerUnequip does NOT fire
        // when items are swapped via right-click, so incremental apply would
        // leave the displaced item's bonuses orphaned → infinite stacking.
        FullRecalcLotteryStats(player);
    }

    void OnPlayerUnequip(Player* player, Item* /*it*/) override
    {
        if (!IsEnabled() || !player) return;
        // Full recalc — handles edge cases where multiple items change in one frame
        FullRecalcLotteryStats(player);
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!IsEnabled() || !player) return;
        LoadLotteryEnchantsForPlayer(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (!IsEnabled() || !player) return;
        UnloadLotteryEnchantsForPlayer(player);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (!IsEnabled() || !player) return;
        RefreshLotteryEnchantsOnLevelUp(player, oldLevel);
    }

    void OnPlayerGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override
    {
        if (!IsEnabled()) return;
        if (menu_id != LOTTERY_GOSSIP_MENU_ID || sender != LOTTERY_GOSSIP_SENDER) return;
        HandleLotteryGossipSelect(player, action);
    }

    // =========================================================================
    // OnPlayerUpdate — Double Jump mechanics + BG flag enforcement.
    //
    // DOUBLE JUMP (Mobility Boost, level <60 or flylock):
    // SetCanFly(true) is set in RecalcLotterySpeedAndFly to enable the space-bar
    // trigger while airborne. When the player presses space while falling, the
    // client transitions to MOVEMENTFLAG_FLYING → IsFlying() becomes true.
    // We detect this, apply an upward knockback, disable CAN_FLY, and mark the
    // double jump as used until the player lands on the ground.
    //
    // FULL FLIGHT (level 60+, not flylock):
    // No intervention needed — player flies normally. Only BG flag enforcement.
    // =========================================================================
    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        if (!IsEnabled() || !player) return;

        bool hasFlyEnchant = player->GetLotteryCanFly();
        bool fullFlight    = player->HasLotteryFullFlight();
        bool doubleJump    = hasFlyEnchant && !fullFlight;
        uint32 pg          = player->GetGUID().GetCounter();

        // --- Double jump mode ---
        if (doubleJump)
        {
            if (player->IsFlying() && !s_doubleJumpUsed[pg])
            {
                // Player pressed space while airborne → client entered "flying" state.
                // Apply upward knockback and remove CAN_FLY to simulate a double jump.
                player->SetCanFly(false);
                player->KnockbackFrom(player->GetPositionX(), player->GetPositionY(), 0.0f, 15.0f);

                // Reset fall tracking so fall damage only counts from the double-jump
                // activation point, not the original cliff edge.
                player->SetFallInformation(getMSTime(), player->GetPositionZ());

                s_doubleJumpUsed[pg] = true;
            }
            else if (!player->IsFalling() && !player->IsFlying() && s_doubleJumpUsed[pg])
            {
                // Player landed on ground — reset double jump for next airborne session.
                s_doubleJumpUsed[pg] = false;
                player->SetCanFly(true);
            }
        }

        // --- BG flag enforcement (applies to both fly modes) ---
        if (player->IsFlying() && player->InBattleground())
        {
            if (player->HasAura(23335) || player->HasAura(23333) || player->HasAura(34976))
            {
                player->RemoveAurasDueToSpell(23335);
                player->RemoveAurasDueToSpell(23333);
                player->RemoveAurasDueToSpell(34976);
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "|cffFF0000You can't carry a flag while flying! Flag dropped.|r");
            }
        }
    }

private:
    static bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
               sConfigMgr->GetOption<bool>("CustomRamvaris.LotteryEnchants.Enable", false);
    }
};

// =============================================================================
// CommandScript: .enchants (gossip), .reroll (gossip), .flylock (toggle)
// =============================================================================
class LotteryEnchants_CommandScript : public CommandScript
{
public:
    LotteryEnchants_CommandScript() : CommandScript("LotteryEnchants_CommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "enchants", HandleEnchantsCommand, SEC_PLAYER, Console::No },
            { "reroll",   HandleRerollCommand,   SEC_PLAYER, Console::No },
            { "flylock",  HandleFlylockCommand,  SEC_PLAYER, Console::No },
        };
        return commandTable;
    }

    // .enchants — Open paginated gossip menu showing all items with lottery enchants
    static bool HandleEnchantsCommand(ChatHandler* handler)
    {
        if (!IsEnabled(handler)) return true;
        Player* player = handler->GetPlayer();
        if (!player) return false;
        ShowEnchantsMenu(player, 0);
        return true;
    }

    // .reroll — Open paginated gossip menu with eligible bag items for reroll
    static bool HandleRerollCommand(ChatHandler* handler)
    {
        if (!IsEnabled(handler)) return true;
        Player* player = handler->GetPlayer();
        if (!player) return false;
        ShowRerollMenu(player, 0);
        return true;
    }

    // .flylock — Toggle between full flight and double jump mode
    static bool HandleFlylockCommand(ChatHandler* handler)
    {
        if (!IsEnabled(handler)) return true;
        Player* player = handler->GetPlayer();
        if (!player) return false;

        if (!player->GetLotteryCanFly())
        {
            handler->PSendSysMessage("|cffFF4444You don't have a Mobility Boost enchant.|r");
            return true;
        }

        bool currentlyLocked = player->IsLotteryFlyLocked();
        player->SetLotteryFlyLocked(!currentlyLocked);

        if (!currentlyLocked)
        {
            // Locking: full flight disabled → double jump mode
            handler->PSendSysMessage("|cffFF8800Mobility Boost: Flight LOCKED → Double Jump mode.|r");
        }
        else
        {
            // Unlocking: full flight (if level 60+) or keep double jump (<60)
            if (player->GetLevel() >= 60)
                handler->PSendSysMessage("|cff00FF00Mobility Boost: Flight UNLOCKED → Full flight active.|r");
            else
                handler->PSendSysMessage("|cff00FF00Mobility Boost: Flight UNLOCKED → Double Jump continues until level 60.|r");
        }
        RecalcLotterySpeedAndFly(player);

        return true;
    }

private:
    static bool IsEnabled(ChatHandler* handler)
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) ||
            !sConfigMgr->GetOption<bool>("CustomRamvaris.LotteryEnchants.Enable", false))
        {
            handler->SendSysMessage("Lottery enchants are not enabled on this server.");
            return false;
        }
        return true;
    }
};

// =============================================================================
// WorldScript: Build pools + ensure DB table on startup
// =============================================================================
class LotteryEnchants_WorldScript : public WorldScript
{
public:
    LotteryEnchants_WorldScript() : WorldScript("LotteryEnchants_WorldScript") { }

    void OnStartup() override
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) ||
            !sConfigMgr->GetOption<bool>("CustomRamvaris.LotteryEnchants.Enable", false))
            return;

        CharacterDatabase.Execute(
            "CREATE TABLE IF NOT EXISTS `character_item_lottery_enchants` ("
            "  `item_guid` INT UNSIGNED NOT NULL,"
            "  `slot_index` TINYINT UNSIGNED NOT NULL,"
            "  `enchant_id` INT UNSIGNED NOT NULL,"
            "  PRIMARY KEY (`item_guid`, `slot_index`)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
        );

        CharacterDatabase.Execute(
            "DELETE le FROM `character_item_lottery_enchants` le "
            "LEFT JOIN `item_instance` ii ON le.`item_guid` = ii.`guid` "
            "WHERE ii.`guid` IS NULL"
        );

        // Clean up duplicate enchants beyond MAX_LOTTERY_SLOTS (slot_index 0-6).
        // Old bug: re-login accumulated enchants. This purges any excess rows.
        CharacterDatabase.Execute(
            "DELETE FROM `character_item_lottery_enchants` WHERE `slot_index` >= 7"
        );

        BuildEnchantPoolsFromDBC();

        LOG_INFO("module", "LotteryEnchants: System initialized — DB-based, up to {} enchants per item, "
                 "level-scaled stats + Movespeed + Mobility Boost enchants.",
                 sConfigMgr->GetOption<uint32>("CustomRamvaris.LotteryEnchants.MaxSlots", MAX_LOTTERY_SLOTS));
    }
};

// =============================================================================
// Script Registration
// =============================================================================
void AddSC_random_enchants()
{
    new LotteryEnchants_PlayerScript();
    new LotteryEnchants_CommandScript();
    new LotteryEnchants_WorldScript();
}
