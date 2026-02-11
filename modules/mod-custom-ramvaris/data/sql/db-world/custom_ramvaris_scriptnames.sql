-- =============================================================================
-- mod-custom-ramvaris: ScriptName and spell bindings
--
-- Sets ScriptName for mod-custom-ramvaris-owned NPCs only.
-- NPCs owned by OTHER modules (mod-reagent-bank, mod-instance-reset,
-- mod-warlock-pet-rename, mod-transmog) keep their original ScriptNames.
-- Flavor text for those NPCs is triggered directly from the .special
-- command handler — no ScriptName needed.
--
-- Also registers the custom Stoneform spell script and quest repeatability.
-- =============================================================================

-- Lilly is mod-custom-ramvaris's own NPC with full gossip + flavor text
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_lilly' WHERE `entry` = 299900;

-- Custom Stoneform for Humans (Spell ID 81013) needs spell_script_names binding
-- so the C++ SpellScript gets loaded when the spell is cast
DELETE FROM `spell_script_names` WHERE `spell_id` = 81013;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (81013, 'spell_human_stoneform_81013');

-- Quest 34700 (The Great Cow Hunting) has QUEST_FLAGS_DAILY (0x1000) in Flags
-- but was missing SpecialFlags = 1 (REPEATABLE), causing a server warning
DELETE FROM `quest_template_addon` WHERE `ID` = 34700;
INSERT INTO `quest_template_addon` (`ID`, `SpecialFlags`) VALUES (34700, 1);
