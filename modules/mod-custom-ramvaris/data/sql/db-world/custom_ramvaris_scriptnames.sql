-- =============================================================================
-- mod-custom-ramvaris: ScriptName bindings for custom NPCs
--
-- Sets the ScriptName field in creature_template so the C++ AI scripts
-- (IsSummonedBy for flavor text, OnGossipHello for Lilly) get loaded.
--
-- These UPDATEs are harmless if the NPC entries don't exist in your DB.
-- =============================================================================

UPDATE `creature_template` SET `ScriptName` = 'npc_custom_ling'        WHERE `entry` = 290011;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_ashari'      WHERE `entry` = 299902;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_lilly'       WHERE `entry` = 299900;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_squeak'      WHERE `entry` = 299901;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_cromi'       WHERE `entry` = 300000;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_ciel'        WHERE `entry` = 299903;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_rename'      WHERE `entry` = 200002;
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_warpweaver'  WHERE `entry` = 190011;
