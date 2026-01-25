-- Soul Keeper Module - Character Database Schema
-- Stores captured creature souls for the guardian system

CREATE TABLE IF NOT EXISTS `character_soul_keeper` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `owner_guid` INT UNSIGNED NOT NULL COMMENT 'Player GUID (low)',
  `creature_entry` INT UNSIGNED NOT NULL COMMENT 'Creature template entry',
  `custom_name` VARCHAR(100) DEFAULT '' COMMENT 'Display name (from creature or custom)',
  `display_id` INT UNSIGNED DEFAULT 0 COMMENT 'Creature display ID for icons',
  `scale_factor` FLOAT DEFAULT 1.0 COMMENT 'Scaling multiplier (reserved)',
  `caught_at` BIGINT UNSIGNED DEFAULT 0 COMMENT 'Unix timestamp of capture',
  PRIMARY KEY (`id`),
  INDEX `idx_owner` (`owner_guid`),
  UNIQUE KEY `unique_soul` (`owner_guid`, `creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Soul Keeper Module - Captured creature souls';
