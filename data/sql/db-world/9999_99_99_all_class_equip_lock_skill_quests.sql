-- ============================================================================
-- AllClassEquip: Lock class skill-learn quests to their correct class
-- ============================================================================
-- Quests that teach class-specific spells via RewardSpell MUST stay locked to
-- the class that can use those spells. In vanilla AzerothCore many of these
-- relied on quest-chain gating (only shamans could reach the totem NPC, etc.)
-- but AllClassEquip breaks those gates. This patch explicitly sets
-- AllowableClasses on every skill-learn quest found in the 3.3.5a DB.
--
-- Class bitmask reference:
--   1 = Warrior    2 = Paladin    4 = Hunter     8 = Rogue
--  16 = Priest    32 = DK        64 = Shaman   128 = Mage
-- 256 = Warlock 1024 = Druid
-- ============================================================================

-- ------------- SHAMAN (64) — Totem quests -----------------------------------
-- Call of Air (RewardSpell 8385) — never had AllowableClasses in vanilla DB
UPDATE quest_template_addon SET AllowableClasses = 64
  WHERE ID IN (1531, 1532, 9554);
-- Call of Earth quest chain precursors (no spell reward, but lead to totem)
-- Already locked by our OnStartup C++ for IDs 1518, 1521, 9451 (RewardSpell 8073)
-- Call of Fire precursors
-- Already locked for IDs 1527, 9555 (RewardSpell 2075)
-- Call of Water precursors
-- Already locked for IDs 96, 9509 (RewardSpell 5396)

-- ------------- HUNTER (4) — Taming quests -----------------------------------
-- Taming the Beast (RewardSpell 1579 = Tame Beast ability)
UPDATE quest_template_addon SET AllowableClasses = 4
  WHERE ID IN (6082, 6085, 6088, 6102, 9485, 9593);

-- ------------- WARLOCK (256) — Summon quests --------------------------------
-- Summon Felsteed (RewardSpell 5785)
UPDATE quest_template_addon SET AllowableClasses = 256
  WHERE ID IN (4490);
-- Dreadsteed of Xoroth (RewardSpell 23160)
UPDATE quest_template_addon SET AllowableClasses = 256
  WHERE ID IN (7631);

-- ------------- PRIEST (16) — Racial priest spells ---------------------------
-- These are priest-only racial abilities. In an AllClassEquip server we lock
-- them to Priest (16) regardless of race, because the spells themselves are
-- priest-class spells.

-- Desperate Prayer (RewardSpell 19338)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5634, 5635, 5636, 5637, 5638, 5639, 5640);
-- Shadowguard (RewardSpell 19331)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5642, 5643, 5680);
-- Hex of Weakness (RewardSpell 19325)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5652, 5654, 5655, 5656, 5657);
-- Touch of Weakness (RewardSpell 19318)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5658, 5660, 5661, 5662, 5663, 10379);
-- Stars of Elune / Returning Home (RewardSpell 19350)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5627, 5628, 5629, 5630, 5631, 5632, 5633);
-- Elune's Grace (RewardSpell 19357)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5672, 5673, 5674, 5675);
-- Arcane Feedback (RewardSpell 19345)
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (5676, 5677, 5678);
-- Symbol of Hope (RewardSpell 32758) — Draenei priest
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (10376);
-- Consume Magic (RewardSpell 32757) — Blood Elf priest
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (10378);
-- Sacred Cloth (RewardSpell 19437) — Priest general
UPDATE quest_template_addon SET AllowableClasses = 16
  WHERE ID IN (6032);
