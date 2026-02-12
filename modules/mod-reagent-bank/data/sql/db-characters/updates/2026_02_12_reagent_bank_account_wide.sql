-- Migration: Convert custom_reagent_bank from character-based to account-based storage
-- Deleted characters (5, 11) with NULL account are assigned to account 1 (RAMIRES)

-- Step 1: Create the new account-wide table
CREATE TABLE IF NOT EXISTS `custom_reagent_bank_new` (
    `account_id` int(11) NOT NULL,
    `item_entry` int(11) NOT NULL,
    `item_subclass` int(11) NOT NULL,
    `amount` int(11) NOT NULL,
    PRIMARY KEY (`account_id`,`item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Step 2: Aggregate data into account-wide entries
-- Map character_id -> account_id via characters table; deleted chars (NULL account) -> account 1
INSERT INTO `custom_reagent_bank_new` (`account_id`, `item_entry`, `item_subclass`, `amount`)
SELECT
    COALESCE(c.account, 1) AS account_id,
    crb.item_entry,
    crb.item_subclass,
    SUM(crb.amount) AS amount
FROM `custom_reagent_bank` crb
LEFT JOIN `characters` c ON crb.character_id = c.guid
GROUP BY COALESCE(c.account, 1), crb.item_entry, crb.item_subclass;

-- Step 3: Drop old table and rename new one
DROP TABLE `custom_reagent_bank`;
ALTER TABLE `custom_reagent_bank_new` RENAME TO `custom_reagent_bank`;
