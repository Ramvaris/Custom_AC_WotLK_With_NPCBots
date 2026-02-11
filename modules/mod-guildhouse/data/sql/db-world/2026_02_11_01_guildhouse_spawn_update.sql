-- Migration: Update guild house profession trainers to Dalaran Grand Master equivalents.
-- Old trainers were low-level or faction-specific NPCs with incomplete spell lists.
-- New trainers are neutral Dalaran Grand Masters — they train ALL recipes up to 450.
-- This update is idempotent: safe to run on both old and already-updated databases.

-- Step 1: Delete faction-specific duplicate rows (Horde variants).
-- JC, Enchanting, and Inscription were spawned twice (Alliance + Horde).
-- Dalaran Grand Masters are neutral — one NPC covers both factions.
DELETE FROM `guild_house_spawns` WHERE `entry` IN (18751, 18753, 30722);

-- Step 2: Update primary profession trainers to Dalaran Grand Masters.
UPDATE `guild_house_spawns` SET `entry` = 28694, `comment` = 'Blacksmithing Trainer' WHERE `entry` = 2836;
UPDATE `guild_house_spawns` SET `entry` = 28698, `comment` = 'Mining Trainer'        WHERE `entry` = 8128;
UPDATE `guild_house_spawns` SET `entry` = 28697, `comment` = 'Engineering Trainer'   WHERE `entry` = 8736;
UPDATE `guild_house_spawns` SET `entry` = 28701, `comment` = 'Jewelcrafting Trainer' WHERE `entry` = 18774;
UPDATE `guild_house_spawns` SET `entry` = 28693, `comment` = 'Enchanting Trainer'    WHERE `entry` = 18773;
UPDATE `guild_house_spawns` SET `entry` = 28702, `comment` = 'Inscription Trainer'   WHERE `entry` = 30721;
UPDATE `guild_house_spawns` SET `entry` = 28700, `comment` = 'Leatherworking Trainer' WHERE `entry` = 19187;
UPDATE `guild_house_spawns` SET `entry` = 28696, `comment` = 'Skinning Trainer'      WHERE `entry` = 19180;
UPDATE `guild_house_spawns` SET `entry` = 28703, `comment` = 'Alchemy Trainer'       WHERE `entry` = 19052;
UPDATE `guild_house_spawns` SET `entry` = 28704, `comment` = 'Herbalism Trainer'     WHERE `entry` = 908;
UPDATE `guild_house_spawns` SET `entry` = 28699, `comment` = 'Tailoring Trainer'     WHERE `entry` = 2627;

-- Step 3: Update secondary profession trainers.
UPDATE `guild_house_spawns` SET `entry` = 28706, `comment` = 'First Aid Trainer'     WHERE `entry` = 19184;
UPDATE `guild_house_spawns` SET `entry` = 28705, `comment` = 'Cooking Trainer'       WHERE `entry` = 19185;
-- Fishing Trainer (entry 2834) remains unchanged.
