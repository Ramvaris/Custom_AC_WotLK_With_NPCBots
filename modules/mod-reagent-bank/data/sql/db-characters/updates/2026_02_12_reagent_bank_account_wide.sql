-- Migration: Convert custom_reagent_bank from character-based to account-based storage
-- Deleted characters (5, 11) with NULL account are assigned to account 1 (RAMIRES)

-- Idempotency:
-- This migration may be re-run in some environments (manual imports, reset updates table,
-- partial failed attempts). We only execute the character->account migration when the old
-- schema column `character_id` exists. If already migrated (`account_id` schema), this file
-- becomes a safe no-op.

SET @has_character_id := (
    SELECT COUNT(*)
    FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'custom_reagent_bank'
      AND COLUMN_NAME = 'character_id'
);

-- Step 1: Create the new account-wide table
DROP TABLE IF EXISTS `custom_reagent_bank_new`;

CREATE TABLE `custom_reagent_bank_new` (
    `account_id` int(11) NOT NULL,
    `item_entry` int(11) NOT NULL,
    `item_subclass` int(11) NOT NULL,
    `amount` int(11) NOT NULL,
    PRIMARY KEY (`account_id`,`item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Step 2: Aggregate data into account-wide entries
-- Map character_id -> account_id via characters table; deleted chars (NULL account) -> account 1
SET @sql := IF(
    @has_character_id > 0,
    'INSERT INTO custom_reagent_bank_new (account_id, item_entry, item_subclass, amount)
     SELECT COALESCE(c.account, 1) AS account_id,
            crb.item_entry,
            crb.item_subclass,
            SUM(crb.amount) AS amount
     FROM custom_reagent_bank crb
     LEFT JOIN characters c ON crb.character_id = c.guid
     GROUP BY COALESCE(c.account, 1), crb.item_entry, crb.item_subclass',
    'DO 0'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Step 3: Drop old table and rename new one
SET @sql := IF(@has_character_id > 0, 'DROP TABLE custom_reagent_bank', 'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @sql := IF(
    @has_character_id > 0,
    'ALTER TABLE custom_reagent_bank_new RENAME TO custom_reagent_bank',
    'DROP TABLE IF EXISTS custom_reagent_bank_new'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_character_id := NULL;
SET @sql := NULL;
