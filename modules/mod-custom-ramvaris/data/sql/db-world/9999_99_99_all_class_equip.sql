-- ============================================================================
-- All-Class Equipment: Remove class restrictions from all weapons and armor
-- Config: CustomRamvaris.AllClassEquip.Enable
--
-- Sets AllowableClass = -1 (all classes) on every weapon and armor piece in
-- item_template. This lets any class equip any item, including class-specific
-- tier sets (Warlock in DK gear, Mage with plate, etc.).
--
-- Also removes AllowableClass restrictions from vendor display/purchase logic
-- (server filters BoP items by AllowableClass before showing them).
--
-- Prefixed 9999_99_99_ to run LAST after all other DB updates.
-- ============================================================================

-- Weapons (class = 2): Remove class restrictions
UPDATE `item_template` SET `AllowableClass` = -1
WHERE `class` = 2 AND `AllowableClass` != -1 AND `AllowableClass` != 0;

-- Armor (class = 4): Remove class restrictions
UPDATE `item_template` SET `AllowableClass` = -1
WHERE `class` = 4 AND `AllowableClass` != -1 AND `AllowableClass` != 0;
