--
-- Lottery Enchant System — Stores unlimited random enchants per item.
-- Decoupled from the 12-slot enchantment system entirely.
-- Stats are applied/unapplied server-side on equip/login.
--
CREATE TABLE IF NOT EXISTS `character_item_lottery_enchants` (
    `item_guid`   INT UNSIGNED NOT NULL COMMENT 'item_instance.guid',
    `slot_index`  TINYINT UNSIGNED NOT NULL COMMENT 'Sequential index: 0, 1, 2, ...',
    `enchant_id`  INT UNSIGNED NOT NULL COMMENT 'SpellItemEnchantment.dbc ID',
    PRIMARY KEY (`item_guid`, `slot_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Lottery enchant system — unlimited random enchants per item';
