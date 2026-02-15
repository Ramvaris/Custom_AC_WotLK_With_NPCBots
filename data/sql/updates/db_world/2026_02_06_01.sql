-- DB update 2026_02_06_00 -> 2026_02_06_01
-- Recalculate quaternion rotation from orientation for unverified gameobjects
-- Excludes type 11 (GAMEOBJECT_TYPE_TRANSPORT / StaticTransport) because their
-- rotation columns store PathRotation (animation path rotation), not WorldRotation
UPDATE `gameobject` g
JOIN `gameobject_template` gt ON g.`id` = gt.`entry`
SET g.`rotation2` = SIN(g.`orientation` / 2), g.`rotation3` = COS(g.`orientation` / 2)
WHERE g.`rotation0` = 0 AND g.`rotation1` = 0
  AND (g.`VerifiedBuild` IS NULL OR g.`VerifiedBuild` = 0)
  AND gt.`type` != 11;
