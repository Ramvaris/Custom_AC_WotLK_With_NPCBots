# mod-custom-ramvaris 🍥

**⚠️ This is a CUSTOM module for Ramvaris' private WotLK 3.3.5a server ONLY**

## What is this?

All of Ramvaris' custom server logic in one C++ module — ported from Lua for performance.
Global toggle ON, all sub-features OFF by default. Safe to ignore or delete for cloners.

## Features

| Feature | Config Toggle | Needs Custom DB? |
|---------|--------------|-----------------|
| **Human Stoneform** (81013) | `HumanStoneform.Enable` | Yes — custom DBC spell |
| **NPC Summon Flavor Text** | `NpcFlavorText.Enable` | Yes — custom creature_template |
| **Lilly Helper NPC** (gossip) | `LillyGossip.Enable` | Yes — custom creature 299900 |
| **Login Spell Grants** | `LoginSpellGrants.Enable` | Yes — custom DBC spells |
| **Dark Azeroth Weather** | `DarkAzeroth.Enable` | No |
| **.special** (NPC summon menu) | `SpecialCommand.Enable` | Yes — custom creature_template |
| **.guardianscale** (SK guardian size) | `GuardianScale.Enable` | No |
| **.petscale** (pet size) | `PetScale.Enable` | No |
| **Pet Stay on Mount** | `PetStayOnMount.Enable` | No — core patch |
| **Lottery Enchants** | `LotteryEnchants.Enable` | Yes — character DB table |

## Config Structure

```ini
# Master switch (loads module but all features stay OFF)
CustomRamvaris.Enable = 1

# Individual feature toggles (all OFF by default)
CustomRamvaris.HumanStoneform.Enable = 0
CustomRamvaris.NpcFlavorText.Enable = 0
CustomRamvaris.LillyGossip.Enable = 0
CustomRamvaris.LoginSpellGrants.Enable = 0
CustomRamvaris.DarkAzeroth.Enable = 0
CustomRamvaris.SpecialCommand.Enable = 0
CustomRamvaris.GuardianScale.Enable = 0
CustomRamvaris.PetScale.Enable = 0
CustomRamvaris.PetStayOnMount.Enable = 0
CustomRamvaris.LotteryEnchants.Enable = 0
CustomRamvaris.LotteryEnchants.MaxSlots = 7
```

## Feature Details

### Human Stoneform (Spell 81013)
Custom racial for Humans — 2 minute cooldown cleanse that removes Poison, Disease, Curse, and Magic debuffs.
Requires DBC entry: `SPELL_EFFECT_DUMMY`, `TARGET_UNIT_CASTER`, `RecoveryTime=120000`.

### NPC Summon Flavor Text
8 custom NPCs each have 12 random witty sayings when summoned:
- **Ling** (290011) — Sarcastic reagent banker
- **Ashari** (299902) — Charismatic vendor
- **Lord Squeak** (299901) — Rat nobleman emblem trader
- **Cromi** (300000) — Time-confused instance resetter
- **Ciel** (299903) — Mount enthusiast
- **Rename** (200002) — Reluctant pet renamer
- **Warpweaver** (190011) — Fashion-obsessed transmogger
- **Lilly** (299900) — Helpful assistant (via LillyGossip toggle)

### Lilly Helper NPC
Full gossip NPC with services:
- **Teleportation** — Cities (faction-aware), Classic dungeons, Secret points of interest
- **Instance Reset** — Unbinds all saved instances for 5 gold
- **Talent Point Purchase** — Exalted reputation + 7 Emblems of Ramires = +1 talent point

### Login Spell Grants
On login, automatically grants utility spells and unlocks profession dual/triple specializations.
Handles Draenei racial fixes (Gift of the Naaru universal version), Blood Elf Warrior Arcane Torrent,
Diplomacy for all, Pick Pocket for all.

**Unlearns on login**: 81002 (Water Form), 81003 (Uber Cheetah), 81008 (Masterful Levitation) —
replaced by the lottery enchant speed/fly system.

### Mend Pet Scale (REMOVED)
Replaced by `.guardianscale` and `.petscale` commands — see below.

### Dark Azeroth Weather
Biome-aware bad weather system — hot zones get thunderstorms, cold zones get snow, others get rain.
5% sunny, 25% light, 30% medium, 40% heavy intensity. Triggers on login and zone change.

### Custom Commands
- `.special` — Gossip menu to summon NPCs, open bank, open mailbox
- `.guardianscale` — Doubles Soul Keeper guardian visual scale each use (caps at 5x base). Only works on Soul Keeper guardians.
- `.petscale` — Doubles any pet visual scale each use (Hunter/Warlock/DK pets, caps at 5x base)
- `.enchants` — Opens paginated gossip menu showing all lottery enchants on equipped + bag items. Click an item to see its enchant details. Color-coded by tier.
- `.reroll` — Opens paginated gossip menu listing eligible bag items (weapons, armor, accessories). Select an item to pay 1000g and reroll enchants. Preview shows rolled stats with Take (apply) / Keep (discard) options. Gold deducted on roll, not on accept.
- `.flylock` — Toggles flight lock. When locked, lottery fly enchants won't grant flight even if you have a flying stage. Useful for avoiding accidental flight in dungeons/raids.

### Pet Stay on Mount
Core patch: pets stay summoned when the player mounts up instead of being dismissed.
Pets run alongside the mounted player. Only affects mount-up — other dismiss triggers
(vehicle entry, teleport, logout) work as normal. Reverts to vanilla behavior when OFF.

### Lottery Enchants
Diablo-style random enchantments on ALL acquired gear (loot, craft, quest, group roll, vendor purchase,
mail). Uses `PLAYERHOOK_ON_STORE_NEW_ITEM` — the universal catch-all from `Player::StoreNewItem`.
Scans SpellItemEnchantment.dbc at startup for pure-stat AND pure-resistance enchants.

**No Class Filtering** — any stat/resist can roll on any class. Self-balancing through randomness:
a warrior rolling INT+SPI+FIRE_RES is a "crap roll" that dilutes power. The dilution IS the balance.
Pool includes primary stats, combat ratings, spell power, haste, crit, hit, expertise, armor pen,
resilience, AND all elemental resistances (Holy/Fire/Nature/Frost/Shadow/Arcane).
Pool **excludes**: Dodge, Parry, Defense (overcapping too easily with multiple items).

Up to 7 enchants per item with cascading chances
(70% → 60% → 50% → 50% → 50% → 40% → 40%). Quality-tiered: item quality caps the max value tier.

**Level Scaling**: Stats grow linearly with player level — `max(1, round(base * level / 80))`.
Level 1 = 1.25%, Level 40 = 50%, Level 80 = 100%. Stats automatically recalculate on level-up
via delta-based application (no full unapply/reapply needed).

#### Speed Enchants
Movespeed is a REGULAR type in the dice pool with equal weight alongside STR, AGI, etc.
Rolls +1% to +25% speed (custom enchant IDs 900001–900025). Stacks additively across all
equipped items, capped at +100% total (200% base speed). Affects MOVE_RUN + MOVE_SWIM.
Multiplicative with aura buffs (e.g., +50% enchant × 1.15 paladin aura = 172.5% speed).
Core patch: Player fields `m_lotterySpeedBonus`, injected into `Unit::UpdateSpeed()` before
final `SetSpeed()` call — survives any aura recalculation. **No level scaling** on speed.

#### Fly Enchants
1% leftover chance before the regular pool. Sub-roll: 75% Stage 1 (100% flight speed),
20% Stage 2 (200%), 5% Stage 3 (300%). Custom enchant IDs 900101–900103.
Only the HIGHEST stage across all equipped items counts. Capped at 600% flight speed.
Flying everywhere — no zone restrictions. BG flag auto-drops when airborne.
Core patch: Player fields `m_lotteryFlySpeedRate` + `m_lotteryCanFly`, injected into
`Unit::UpdateSpeed(MOVE_FLIGHT)`. Slow Fall (spell 130) cast on fly disable to prevent death.
**No level scaling** on flight. Use `.flylock` to voluntarily disable flight.

#### Reroll (Gossip Menu)
`.reroll` opens a paginated gossip menu listing eligible bag items. Select an item → pay 1000g →
enchants are rolled and previewed. Choose **Take** (apply enchants, set 777 durability) or
**Keep** (discard new roll, keep existing enchants). Gold deducted on roll, NOT on accept —
prevents reopen abuse. First enchant guaranteed (100%), rest use normal cascade.

#### Enchant Viewer (Gossip Menu)
`.enchants` opens a paginated gossip menu showing all items with lottery enchants (equipped + bags).
Click an item to drill down into its enchant details. Color-coded by tier:
- **Item name color** (by enchant count): Grey(1), White(2), Green(3), Blue(4), Purple(5), Orange(6), Red(7)
- **Stat value color** (by percentile): Grey → White → Green → Blue → Purple → Orange → Red (~14.29% each)

Enchants stored in custom DB table `character_item_lottery_enchants` — completely decoupled from
item enchantment slots. Stacks additively with profession enchants (no conflict). Server applies
stats on equip, character sheet shows correct totals. Items with lottery enchants get 777/777
durability as a visual marker.

## SQL Setup

SQL runs automatically via AzerothCore's UpdateFetcher system.
Files in `data/sql/db-world/base/` are applied on first server start.
UPDATEs are harmless if the NPC entries don't exist in your DB.

## Lua Archive

The `lua/` directory contains the original Lua scripts (else.lua, lilly.lua, dark_azeroth.lua,
commands.lua) that were ported to C++. They are kept for reference only — ALE no longer loads them.

## For Cloners

- This module is a "dead module" — it won't break anything but won't do anything useful
- You can safely delete the entire `modules/mod-custom-ramvaris/` folder
- The custom spell IDs (81005, 81007, 81011, 81013) don't exist in standard DBC
- Spells 81002 (Water Form), 81003 (Uber Cheetah), 81008 (Levitation) are actively unlearned on login
- Custom enchant IDs 900001–900025 (Speed) and 900101–900103 (Fly) are module-internal, not in DBC
- Core patches: Player.h (lottery speed/fly fields), Unit.cpp (UpdateSpeed injection)
- The custom NPC entries (290011, 299900-299903, 300000, 200002, 190011) don't exist in standard DB

## License

Same as AzerothCore — GNU AGPL v3
