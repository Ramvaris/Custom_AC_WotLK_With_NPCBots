-- ============================================================================
-- Soul Keeper Migration: Character GUID → Account ID
-- SAFE for both existing servers AND fresh installs.
-- On fresh installs (account_id already exists, owner_guid doesn't), skips everything.
-- On existing servers (owner_guid exists), performs the full migration.
--
-- What this does (ONLY if owner_guid column exists):
-- 1. Adds account_id column
-- 2. Populates it by joining characters table (owner_guid → account)
-- 3. Removes duplicate souls (same account + creature_entry from different alts)
--    Keeps the entry with the EARLIEST caught_at timestamp
-- 4. Drops old owner_guid column and rebuilds indexes
-- ============================================================================

-- Wrap in a stored procedure so we can use IF/THEN logic for DDL
DELIMITER //

DROP PROCEDURE IF EXISTS `soul_keeper_migrate_to_account` //

CREATE PROCEDURE `soul_keeper_migrate_to_account`()
BEGIN
    -- Check if the old schema exists (owner_guid column present)
    -- If it doesn't exist, this is a fresh install — nothing to do.
    IF EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_schema = DATABASE()
          AND table_name = 'character_soul_keeper'
          AND column_name = 'owner_guid'
    ) THEN

        -- Step 1: Add account_id column
        IF NOT EXISTS (
            SELECT 1 FROM information_schema.columns
            WHERE table_schema = DATABASE()
              AND table_name = 'character_soul_keeper'
              AND column_name = 'account_id'
        ) THEN
            ALTER TABLE `character_soul_keeper`
              ADD COLUMN `account_id` INT UNSIGNED NOT NULL DEFAULT 0
              COMMENT 'Account ID from auth.account'
              AFTER `id`;
        END IF;

        -- Step 2: Populate account_id from characters table
        UPDATE `character_soul_keeper` sk
          INNER JOIN `characters` c ON sk.`owner_guid` = c.`guid`
          SET sk.`account_id` = c.`account`
          WHERE sk.`account_id` = 0;

        -- Step 3: Delete orphaned rows (characters that no longer exist)
        DELETE FROM `character_soul_keeper` WHERE `account_id` = 0;

        -- Step 4: Remove duplicate souls across alts on the same account
        -- Keep only the EARLIEST capture (lowest caught_at). Delete the rest.
        DELETE sk FROM `character_soul_keeper` sk
          INNER JOIN (
            SELECT `account_id`, `creature_entry`, MIN(`caught_at`) AS first_catch
            FROM `character_soul_keeper`
            GROUP BY `account_id`, `creature_entry`
            HAVING COUNT(*) > 1
          ) dupes ON sk.`account_id` = dupes.`account_id`
                 AND sk.`creature_entry` = dupes.`creature_entry`
                 AND sk.`caught_at` > dupes.`first_catch`;

        -- Step 4b: Handle edge case where duplicate entries have the SAME caught_at
        DELETE sk FROM `character_soul_keeper` sk
          INNER JOIN (
            SELECT `account_id`, `creature_entry`, MIN(`id`) AS keep_id
            FROM `character_soul_keeper`
            GROUP BY `account_id`, `creature_entry`
            HAVING COUNT(*) > 1
          ) dupes ON sk.`account_id` = dupes.`account_id`
                 AND sk.`creature_entry` = dupes.`creature_entry`
                 AND sk.`id` > dupes.`keep_id`;

        -- Step 5: Drop old column and indexes, rebuild with account_id
        -- Use dynamic SQL for conditional index drops
        IF EXISTS (
            SELECT 1 FROM information_schema.statistics
            WHERE table_schema = DATABASE()
              AND table_name = 'character_soul_keeper'
              AND index_name = 'idx_owner'
        ) THEN
            DROP INDEX `idx_owner` ON `character_soul_keeper`;
        END IF;

        IF EXISTS (
            SELECT 1 FROM information_schema.statistics
            WHERE table_schema = DATABASE()
              AND table_name = 'character_soul_keeper'
              AND index_name = 'unique_soul'
        ) THEN
            DROP INDEX `unique_soul` ON `character_soul_keeper`;
        END IF;

        ALTER TABLE `character_soul_keeper` DROP COLUMN `owner_guid`;

        -- Rebuild indexes for new schema
        IF NOT EXISTS (
            SELECT 1 FROM information_schema.statistics
            WHERE table_schema = DATABASE()
              AND table_name = 'character_soul_keeper'
              AND index_name = 'idx_account'
        ) THEN
            ALTER TABLE `character_soul_keeper`
              ADD INDEX `idx_account` (`account_id`);
        END IF;

        IF NOT EXISTS (
            SELECT 1 FROM information_schema.statistics
            WHERE table_schema = DATABASE()
              AND table_name = 'character_soul_keeper'
              AND index_name = 'unique_soul'
        ) THEN
            ALTER TABLE `character_soul_keeper`
              ADD UNIQUE KEY `unique_soul` (`account_id`, `creature_entry`);
        END IF;

    END IF;
    -- If owner_guid doesn't exist → fresh install, new schema already active. Nothing to do.
END //

DELIMITER ;

-- Execute and clean up
CALL `soul_keeper_migrate_to_account`();
DROP PROCEDURE IF EXISTS `soul_keeper_migrate_to_account`;
