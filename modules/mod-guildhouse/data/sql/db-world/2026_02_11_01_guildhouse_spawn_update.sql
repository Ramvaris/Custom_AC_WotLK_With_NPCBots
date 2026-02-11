-- Migration: Update guild house profession trainers to Dalaran Grand Master equivalents.
-- Old trainers were low-level or faction-specific NPCs with incomplete spell lists.
-- New trainers are neutral Dalaran Grand Masters — they train ALL recipes up to 450.
--
-- Idempotent: safe on fresh DBs (already Dalaran), old DBs (needs migration),
-- and partially-migrated DBs (base file re-ran via REPLACE INTO by `id`,
-- leaving orphaned old rows alongside new ones).

-- Step 1: Purge ALL old trainer entries.
-- Removes old primary + faction-specific duplicates. No-op if already gone.
DELETE FROM `guild_house_spawns` WHERE `entry` IN (
    2836,  -- old Blacksmithing
    8128,  -- old Mining
    8736,  -- old Engineering
    18774, -- old JC (Alliance)
    18751, -- old JC (Horde)
    18773, -- old Enchanting (Alliance)
    18753, -- old Enchanting (Horde)
    30721, -- old Inscription (Alliance)
    30722, -- old Inscription (Horde)
    19187, -- old Leatherworking
    19180, -- old Skinning
    19052, -- old Alchemy
    908,   -- old Herbalism
    2627,  -- old Tailoring
    19184, -- old First Aid
    19185  -- old Cooking
);

-- Step 2: Ensure Dalaran Grand Master trainers exist at correct positions.
-- REPLACE INTO: overwrites by PRIMARY KEY (`id`) if row exists, inserts otherwise.
-- This handles fresh DBs (already correct), old DBs (old rows deleted above),
-- and mixed-state DBs (some new entries from base file re-run).
REPLACE INTO `guild_house_spawns` (`id`, `entry`, `posX`, `posY`, `posZ`, `orientation`, `comment`) VALUES
    (13, 28694, 16220.5, 16302.3, 13.176,  6.14647, 'Blacksmithing Trainer'),
    (14, 28698, 16220.2, 16299.6, 13.178,  6.22894, 'Mining Trainer'),
    (15, 28697, 16219.8, 16296.9, 13.1746, 6.24465, 'Engineering Trainer'),
    (16, 28701, 16222.4, 16293,   13.1813, 1.51263, 'Jewelcrafting Trainer'),
    (17, 28693, 16227.5, 16292.3, 13.1839, 1.49691, 'Enchanting Trainer'),
    (18, 28702, 16231.6, 16301,   13.1757, 3.07372, 'Inscription Trainer'),
    (19, 28700, 16231.2, 16295,   13.1761, 3.06574, 'Leatherworking Trainer'),
    (20, 28696, 16228.9, 16304.7, 13.1819, 4.64831, 'Skinning Trainer'),
    (21, 28703, 16218.1, 16281.8, 13.1756, 6.1975,  'Alchemy Trainer'),
    (22, 28704, 16218.3, 16284.3, 13.1756, 6.1975,  'Herbalism Trainer'),
    (23, 28699, 16220.4, 16278.7, 13.1756, 1.46157, 'Tailoring Trainer'),
    (24, 28706, 16225,   16310.9, 29.262,  6.22119, 'First Aid Trainer'),
    (26, 28705, 16227,   16278,   13.1762, 1.4872,  'Cooking Trainer');
