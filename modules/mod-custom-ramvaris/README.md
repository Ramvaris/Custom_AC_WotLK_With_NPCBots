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
| **.mountup** (auto-mount) | `MountUp.Enable` | No |
| **.guardianscale** (SK guardian size) | `GuardianScale.Enable` | No |
| **.petscale** (pet size) | `PetScale.Enable` | No |
| **Pet Stay on Mount** | `PetStayOnMount.Enable` | No — core patch |

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
CustomRamvaris.MountUp.Enable = 0
CustomRamvaris.GuardianScale.Enable = 0
CustomRamvaris.PetScale.Enable = 0
CustomRamvaris.PetStayOnMount.Enable = 0
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

### Mend Pet Scale (REMOVED)
Replaced by `.guardianscale` and `.petscale` commands — see below.

### Dark Azeroth Weather
Biome-aware bad weather system — hot zones get thunderstorms, cold zones get snow, others get rain.
5% sunny, 25% light, 30% medium, 40% heavy intensity. Triggers on login and zone change.

### Custom Commands
- `.special` — Gossip menu to summon NPCs, open bank, open mailbox
- `.mountup` — Auto-mount based on riding skill (flying in Outland/Northrend, ground elsewhere)
- `.guardianscale` — Doubles Soul Keeper guardian visual scale each use (caps at 5x base). Only works on Soul Keeper guardians.
- `.petscale` — Doubles any pet visual scale each use (Hunter/Warlock/DK pets, caps at 5x base)

### Pet Stay on Mount
Core patch: pets stay summoned when the player mounts up instead of being dismissed.
Pets run alongside the mounted player. Only affects mount-up — other dismiss triggers
(vehicle entry, teleport, logout) work as normal. Reverts to vanilla behavior when OFF.

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
- The custom spell IDs (81002, 81003, 81005, 81007, 81008, 81011, 81013) don't exist in standard DBC
- The custom NPC entries (290011, 299900-299903, 300000, 200002, 190011) don't exist in standard DB

## License

Same as AzerothCore — GNU AGPL v3
