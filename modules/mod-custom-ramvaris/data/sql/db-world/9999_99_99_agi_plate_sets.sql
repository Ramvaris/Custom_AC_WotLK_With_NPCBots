-- ============================================================================
-- Agility Plate Sets — Custom items for AGI classes wearing plate
-- Config: CustomRamvaris.AllClassEquip.Enable
--
-- Two sets cloned from existing STR plate, with Strength swapped to Agility:
--
-- SET 1: "Nightstalker's Battlegear" (ilvl 264, T10 equivalent)
--   Cloned from Sanctified Ymirjar Lord's DPS set (itemset 895)
--   Entries: 80001-80005, custom itemset 9001
--
-- SET 2: "Phantom Dreadnaught Battlegear" (ilvl 200, T7 equivalent)
--   Cloned from Heroes' Dreadnaught DPS set (itemset 788)
--   Entries: 80006-80010, custom itemset 9002
--
-- Both sets use the same visual models as their source items (no DBC edits).
-- item_dbc entries register the items with the server's DBC store.
-- Sold by Ashari (NPC 299902).
--
-- Re-applied every startup by AllClassEquip WorldScript — safe against wipes.
-- ============================================================================

-- ============================================================================
-- Phase 1: item_dbc entries (register items in the DBC store)
-- Required or ObjectMgr::LoadItemTemplates() silently skips them.
-- Format: ID, ClassID, SubclassID, Sound_Override_Subclassid, Material, DisplayInfoID, InventoryType, SheatheType
-- ============================================================================

DELETE FROM item_dbc WHERE ID BETWEEN 80001 AND 80010;
INSERT INTO item_dbc (ID, ClassID, SubclassID, Sound_Override_Subclassid, Material, DisplayInfoID, InventoryType, SheatheType) VALUES
-- Nightstalker's (T10 clone — same visuals as Sanctified Ymirjar Lord's)
(80001, 4, 4, -1, 4, 64570, 1, 0),  -- Helmet   (Head)
(80002, 4, 4, -1, 6, 64622, 3, 0),  -- Mantle   (Shoulder)
(80003, 4, 4, -1, 1, 64569, 5, 0),  -- Hauberk  (Chest)
(80004, 4, 4, -1, 6, 64572, 7, 0),  -- Legguards (Legs)
(80005, 4, 4, -1, 1, 64571, 10, 0), -- Grips    (Hands)
-- Phantom Dreadnaught (T7 clone — same visuals as Heroes' Dreadnaught)
(80006, 4, 4, -1, 4, 54403, 1, 0),  -- Helmet   (Head)
(80007, 4, 4, -1, 6, 56214, 3, 0),  -- Mantle   (Shoulder)
(80008, 4, 4, -1, 1, 55369, 5, 0),  -- Hauberk  (Chest)
(80009, 4, 4, -1, 6, 55376, 7, 0),  -- Legguards (Legs)
(80010, 4, 4, -1, 1, 55371, 10, 0); -- Grips    (Hands)

-- ============================================================================
-- Phase 2: item_template entries (full item data)
-- Cloned 1:1 from source items with these changes:
--   - entry: 80001-80010
--   - name: creative AGI-themed names
--   - stat_type1: 4 (STR) → 3 (AGI) — same values, just the stat type
--   - itemset: custom sets 9001/9002 (no set bonuses in DBC = no set bonus UI)
--   - AllowableClass: -1 (all classes)
--   - description: flavor text
-- ============================================================================

-- Clean slate (idempotent)
DELETE FROM item_template WHERE entry BETWEEN 80001 AND 80010;

-- ---------------------------------------------------------------------------
-- SET 1: Nightstalker's Battlegear (ilvl 264) — cloned from T10 Warrior DPS
-- Source: Sanctified Ymirjar Lord's (itemset 895)
-- stat_type1 changed: 4 (STR) → 3 (AGI)
-- ---------------------------------------------------------------------------

-- 80001: Nightstalker's Visage (Head) — from 51212
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80001, class, subclass, SoundOverrideSubclass,
    'Nightstalker''s Visage',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,  -- AGI instead of STR, same value
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'Forged in shadow for those who favor speed over brute force.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9001, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 51212;

-- 80002: Nightstalker's Mantle (Shoulder) — from 51210
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80002, class, subclass, SoundOverrideSubclass,
    'Nightstalker''s Mantle',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'Forged in shadow for those who favor speed over brute force.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9001, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 51210;

-- 80003: Nightstalker's Hauberk (Chest) — from 51214
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80003, class, subclass, SoundOverrideSubclass,
    'Nightstalker''s Hauberk',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'Forged in shadow for those who favor speed over brute force.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9001, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 51214;

-- 80004: Nightstalker's Legguards (Legs) — from 51211
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80004, class, subclass, SoundOverrideSubclass,
    'Nightstalker''s Legguards',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'Forged in shadow for those who favor speed over brute force.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9001, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 51211;

-- 80005: Nightstalker's Grips (Hands) — from 51213
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80005, class, subclass, SoundOverrideSubclass,
    'Nightstalker''s Grips',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'Forged in shadow for those who favor speed over brute force.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9001, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 51213;

-- ---------------------------------------------------------------------------
-- SET 2: Phantom Dreadnaught Battlegear (ilvl 200) — cloned from T7 Warrior DPS
-- Source: Heroes' Dreadnaught (itemset 788)
-- stat_type1 changed: 4 (STR) → 3 (AGI)
-- ---------------------------------------------------------------------------

-- 80006: Phantom Dreadnaught Helmet (Head) — from 39605
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80006, class, subclass, SoundOverrideSubclass,
    'Phantom Dreadnaught Helmet',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'An echo of battle past, reforged for the swift.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9002, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 39605;

-- 80007: Phantom Dreadnaught Shoulderplates (Shoulder) — from 39608
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80007, class, subclass, SoundOverrideSubclass,
    'Phantom Dreadnaught Shoulderplates',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'An echo of battle past, reforged for the swift.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9002, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 39608;

-- 80008: Phantom Dreadnaught Battleplate (Chest) — from 39606
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80008, class, subclass, SoundOverrideSubclass,
    'Phantom Dreadnaught Battleplate',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'An echo of battle past, reforged for the swift.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9002, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 39606;

-- 80009: Phantom Dreadnaught Legplates (Legs) — from 39607
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80009, class, subclass, SoundOverrideSubclass,
    'Phantom Dreadnaught Legplates',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'An echo of battle past, reforged for the swift.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9002, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 39607;

-- 80010: Phantom Dreadnaught Gauntlets (Hands) — from 39609
INSERT INTO item_template (entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, AllowableClass, AllowableRace,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, description, PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    itemset, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom)
SELECT
    80010, class, subclass, SoundOverrideSubclass,
    'Phantom Dreadnaught Gauntlets',
    displayid, Quality, Flags, FlagsExtra,
    BuyCount, BuyPrice, SellPrice, InventoryType, -1, -1,
    ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell,
    requiredhonorrank, RequiredCityRank, RequiredReputationFaction, RequiredReputationRank,
    maxcount, stackable, ContainerSlots,
    3, stat_value1,
    stat_type2, stat_value2, stat_type3, stat_value3,
    stat_type4, stat_value4, stat_type5, stat_value5,
    stat_type6, stat_value6, stat_type7, stat_value7, stat_type8, stat_value8,
    stat_type9, stat_value9, stat_type10, stat_value10,
    ScalingStatDistribution, ScalingStatValue,
    dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,
    armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res,
    delay, ammo_type, RangedModRange,
    spellid_1, spelltrigger_1, spellcharges_1, spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1,
    spellid_2, spelltrigger_2, spellcharges_2, spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2,
    spellid_3, spelltrigger_3, spellcharges_3, spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3,
    spellid_4, spelltrigger_4, spellcharges_4, spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4,
    spellid_5, spelltrigger_5, spellcharges_5, spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5,
    bonding, 'An echo of battle past, reforged for the swift.',
    PageText, LanguageID, PageMaterial, startquest,
    lockid, Material, sheath, RandomProperty, RandomSuffix, block,
    9002, MaxDurability, area, Map, BagFamily, TotemCategory,
    socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3,
    socketBonus, GemProperties, RequiredDisenchantSkill, ArmorDamageModifier,
    duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID,
    FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom
FROM item_template WHERE entry = 39609;

-- ============================================================================
-- Phase 3: Add to Ashari's vendor inventory (NPC 299902)
-- ============================================================================

DELETE FROM npc_vendor WHERE entry = 299902 AND item BETWEEN 80001 AND 80010;
INSERT INTO npc_vendor (entry, slot, item, maxcount, incrtime, ExtendedCost) VALUES
-- Nightstalker's Battlegear (T10 AGI plate)
(299902, 0, 80001, 0, 0, 0),  -- Visage (Head)
(299902, 0, 80002, 0, 0, 0),  -- Mantle (Shoulder)
(299902, 0, 80003, 0, 0, 0),  -- Hauberk (Chest)
(299902, 0, 80004, 0, 0, 0),  -- Legguards (Legs)
(299902, 0, 80005, 0, 0, 0),  -- Grips (Hands)
-- Phantom Dreadnaught Battlegear (T7 AGI plate)
(299902, 0, 80006, 0, 0, 0),  -- Helmet (Head)
(299902, 0, 80007, 0, 0, 0),  -- Shoulderplates (Shoulder)
(299902, 0, 80008, 0, 0, 0),  -- Battleplate (Chest)
(299902, 0, 80009, 0, 0, 0),  -- Legplates (Legs)
(299902, 0, 80010, 0, 0, 0);  -- Gauntlets (Hands)
