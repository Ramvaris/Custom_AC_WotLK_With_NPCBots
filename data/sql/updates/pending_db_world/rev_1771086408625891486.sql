-- fix(DB/Gameobject): Restore StaticTransport (type 11) PathRotation values
-- The quaternion recalculation in 2026_02_06_01 incorrectly overwrote
-- rotation2/rotation3 for StaticTransport entries. For type 11 gameobjects,
-- these columns store PathRotation (animation path rotation), NOT WorldRotation.
-- The tram (entries 176080-176086) had PathRotation (0,0,1,0) = 180 degrees,
-- which was wrongly recalculated to (0,0,±0.7071,0.7071) = ±90 degrees,
-- causing the Deeprun Tram to travel in the wrong direction.
--
-- Upstream fix (2026_02_10_01) added gameobject_addon parent_rotation for 10 GUIDs
-- and changed Transport.cpp to prefer addon over gameobject.rotation.
-- This patch covers the remaining 16 affected GUIDs (both rotation fallback + addon).

-- Restore gameobject.rotation2/rotation3 for ALL 26 affected StaticTransport spawns
-- (acts as fallback for code paths that read rotation before addon)

-- Deeprun Tram carts (map 369) - entries 176080-176086
UPDATE `gameobject` SET `rotation2` = 1, `rotation3` = 0 WHERE `guid` IN (18802, 18803, 18804, 18805, 18806, 18807);

-- Other affected StaticTransport entries with incorrect recalculated rotations
UPDATE `gameobject` SET `rotation2` = 0.996917, `rotation3` = -0.078459 WHERE `guid` IN (2837, 58306);
UPDATE `gameobject` SET `rotation2` = 0.992005, `rotation3` = -0.126199 WHERE `guid` = 6946;
UPDATE `gameobject` SET `rotation2` = 1, `rotation3` = 0.001 WHERE `guid` = 13487;
UPDATE `gameobject` SET `rotation2` = 0.999048, `rotation3` = 0.043619 WHERE `guid` IN (55230, 56961, 57799, 60463);
UPDATE `gameobject` SET `rotation2` = 1, `rotation3` = 0 WHERE `guid` IN (56162, 56163);
UPDATE `gameobject` SET `rotation2` = 0.951057, `rotation3` = 0.309017 WHERE `guid` IN (56937, 56954);
UPDATE `gameobject` SET `rotation2` = 0.932008, `rotation3` = -0.362438 WHERE `guid` = 58935;
UPDATE `gameobject` SET `rotation2` = 0.99999, `rotation3` = 0.004363 WHERE `guid` IN (59160, 65436);
UPDATE `gameobject` SET `rotation2` = 0.99999, `rotation3` = -0.004363 WHERE `guid` IN (59328, 59336, 59343, 59350, 59386);

-- Add gameobject_addon parent_rotation for the 16 GUIDs not covered by upstream's 2026_02_10_01
-- (upstream already covers: 2837, 6946, 18802-18807, 56162, 56163)
DELETE FROM `gameobject_addon` WHERE `guid` IN (13487,55230,56937,56954,56961,57799,58306,58935,59160,59328,59336,59343,59350,59386,60463,65436);
INSERT INTO `gameobject_addon` (`guid`,`parent_rotation0`,`parent_rotation1`,`parent_rotation2`,`parent_rotation3`,`invisibilityType`,`invisibilityValue`) VALUES
(13487,0,0,1,0.001,0,0),
(55230,0,0,0.999048,0.043619,0,0),
(56937,0,0,0.951057,0.309017,0,0),
(56954,0,0,0.951057,0.309017,0,0),
(56961,0,0,0.999048,0.043619,0,0),
(57799,0,0,0.999048,0.043619,0,0),
(58306,0,0,0.996917,-0.078459,0,0),
(58935,0,0,0.932008,-0.362438,0,0),
(59160,0,0,0.99999,0.004363,0,0),
(59328,0,0,0.99999,-0.004363,0,0),
(59336,0,0,0.99999,-0.004363,0,0),
(59343,0,0,0.99999,-0.004363,0,0),
(59350,0,0,0.99999,-0.004363,0,0),
(59386,0,0,0.99999,-0.004363,0,0),
(60463,0,0,0.999048,0.043619,0,0),
(65436,0,0,0.99999,0.004363,0,0);
