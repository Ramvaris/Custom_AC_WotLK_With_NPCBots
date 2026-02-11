# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)
## Build Status

3.3.5
:------------:
[![nopch-build](https://github.com/trickerer/AzerothCore-wotlk-with-NPCBots/actions/workflows/core-build-nopch.yml/badge.svg?branch=npcbots_3.3.5)](https://github.com/trickerer/AzerothCore-wotlk-with-NPCBots/actions/workflows/core-build-nopch.yml)
[![windows-build](https://github.com/trickerer/AzerothCore-wotlk-with-NPCBots/actions/workflows/windows_build.yml/badge.svg)](https://github.com/trickerer/AzerothCore-wotlk-with-NPCBots/actions/workflows/windows_build.yml)
[![dashboard-ci](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/dashboard-ci.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/dashboard-ci.yml?query=branch%3Amaster)

## Custom

This fork is maintained by **[Ramvaris](https://github.com/Ramvaris)**.

- Custom Soul Collector Mod (My Own Invention, with the help of my AI Agent Ryo) [Does NOT work without some AC core file mods done here!]
- Custom Solo Sustain Mod for life/mana leech (damage-based, class-specific, Recount/MSBT compatible)
- A number of 'defensive crash protectors' for the server:
  - **SmartAI Recursion Gate** - Prevents stack overflow from infinite SmartAI script loops (depth-limited with logging)
  - **Aura System Hardening** - Converts fatal ABORT() calls to graceful LOG_ERROR recovery when aura maps desync
  - **Spell Event Safety** - Non-deletable spell events are force-cleaned up instead of crashing + leaking memory
  - **Charm State Cleanup** - Stale charmer GUIDs are force-cleared on world removal instead of crashing
  - **Minion Ownership Safety** - Owner GUID mismatches in controlled sets are logged and skipped, not crashed
  - **Warlock Bot Life Tap Fix** - Prevents NPCBot warlocks from killing themselves with Life Tap (capped health cost + alive guard)
  - **Aura Duration Zero-Tick Fix** - Prevents periodic aura ticks from firing when duration has already reached zero (changed `>= 0` to `> 0` in AuraEffect::Update). Fixes debuffs visually at 0s but still ticking for several seconds under server load.
- Some QoL stuff
-> Looting of mobs that were killed by a players pet without the player doing damage to it
-> All Ore- and Flower-Nodes respawn in 120 seconds instead of 24 hours to 7 days.
- **Working Language Comprehension** - NPCs speaking Orcish/Thalassian/etc. are actually readable if your character knows the language (via skill or aura). Non-speakers get the original scrambled text with proper language tags. No Client-Mod neccesary.
- **Pet Stay on Mount** - Pets stay summoned when mounting up instead of being dismissed. They run alongside the player. Config-gated (`CustomRamvaris.PetStayOnMount.Enable`).
- **Warrior Stance Freedom** - All 228 warrior spells and talents have been stance-neutralized in Spell.dbc. Warriors can use ANY ability in ANY stance (Battle/Defensive/Berserker). Non-warrior forms (Druid Bear/Cat, Rogue Stealth) are untouched. Applied via `tools/warrior_stance_neutralizer.py`. Client-side DBC must also be patched (user responsibility).
- **Soul Keeper Guardian Travel Safety** - Guardians are automatically despawned on cross-map teleport and taxi flights, preventing orphaned guardians on old maps. Cooldowns are preserved for re-summoning.
- Some AC modules from other people integrated and kept up-to-date

Notice: I also have some MPQ QoL Stuff that I couldn't include here. Like a custom faction with 'endless talent points' if you can pay massive amounts of gold, QoL Spells (Detect Invisibility to see easter eggs with endless duration, Stealth without Movement Speed Reduction), Human Reputation Bonus for everyone on Login, an ARAC (All Races all Classes) LUA Fix to give dreaenei and blood elves the right class racials, and stuff I have yet forgotten - so if you are interested just ask.

### Included Modules

These modules are from the amazing AzerothCore community. Full credit to the original authors!

| Module | Description | Source |
|--------|-------------|--------|
| **mod-autobalance** | Automatically scales dungeon/raid difficulty based on group size | [azerothcore/mod-autobalance](https://github.com/azerothcore/mod-autobalance) |
| **mod-transmog** | Transmogrification system for appearance changes | [azerothcore/mod-transmog](https://github.com/azerothcore/mod-transmog) |
| **mod-solo-lfg** | Enables solo queuing for dungeons via LFG | [azerothcore/mod-solo-lfg](https://github.com/azerothcore/mod-solo-lfg) |
| **mod-guildhouse** | Personal guild housing system | [azerothcore/mod-guildhouse](https://github.com/azerothcore/mod-guildhouse) |
| **mod-instance-reset** | Extended instance reset options | [azerothcore/mod-instance-reset](https://github.com/azerothcore/mod-instance-reset) |
| **mod-reagent-bank** | Additional storage for crafting reagents | [azerothcore/mod-reagent-bank](https://github.com/azerothcore/mod-reagent-bank) |
| **mod-skip-dk-starting-area** | Skip the Death Knight starting zone | [azerothcore/mod-skip-dk-starting-area](https://github.com/azerothcore/mod-skip-dk-starting-area) |
| **mod-ale** | AzerothCore Lua Engine for scripting | [azerothcore/mod-eluna](https://github.com/azerothcore/mod-eluna) |
| **mod-gain-honor-guard** | Honor gain adjustments | [azerothcore/mod-gain-honor-guard](https://github.com/azerothcore/mod-gain-honor-guard) |
| **mod-pvp-titles** | PvP ranking and title system | [azerothcore/mod-pvp-titles](https://github.com/azerothcore/mod-pvp-titles) |
| **mod-warlock-pet-rename** | Allow Warlocks to rename their pets | [azerothcore/mod-individual-progression](https://github.com/azerothcore/mod-individual-progression) |

### 🍥 Custom Modules (Made for this fork)

| Module | Description |
|--------|-------------|
| **mod-soul-keeper** | Capture creature souls and summon them as intelligent guardians — account-based collection shared across all characters! |
| **mod-solo-sustain** | Damage-based life/mana leech for solo play - Recount/MSBT compatible! |
| **mod-custom-ramvaris** | Private server customizations — custom spells, NPC gossip, dark weather, utility commands, random enchants |

#### Soul Keeper Features
**Soul Capture & Summoning:**
- Capture any dead creature's soul (including elites, bosses, rares)
- Summon captured souls as combat guardians that fight alongside you
- **Account-based collection** — all characters on the same account share one soul library
- Guardians scale with player level and stats
- Custom naming support - rename your guardians!

**Intelligent Guardian AI:**
- **Native AI preserved** - Guardians use their creature abilities (CombatAI, SmartAI, ScriptedAI)
- **Support AI injection** - Adds healing, buffing, and dispelling without replacing native behavior
- **Combat healing** - Emergency heals at 35% HP, normal heals at 60% HP
- **Combat buffs** - Guardians cast Bloodlust, Battle Shout, etc. during combat
- **Out-of-combat healing** - Guardians heal you and themselves to 95% HP when idle
- **Out-of-combat buffs** - Apply missing buffs before the next fight
- **Dispel support** - Removes Magic, Curse, Disease, Poison from owner AND self
- **Mana-aware** - Guardians check mana cost before casting (scaled by level!)
- **Smart buff spam prevention** - Soul Keeper guardians don't recast self-buffs already active (Frost Ward, etc.)
- **External buffs allowed** - Spells that can target others (Mark of the Wild) are never blocked
- **Guardian-only** - Core CombatAI fix uses marker check, no changes to vanilla creature behavior
- **No AI conflicts** - UNIT_STATE checks prevent double-casting with native AI

**Gossip Menu (Soul Collection):**
- **Fast navigation** - Next Page button at TOP for quick browsing through 10+ pages
- **15 souls per page** - More souls visible, less page flipping
- **Clean interface** - No duplicate buttons for commands that already exist

**Guardian Scaling System:**
All guardian output scales with YOUR gear and level - no overpowered captured bosses!

| Damage/Heal Type | Formula | Notes |
|------------------|---------|-------|
| Melee attacks | DPS × attack_speed | Uses creature's native swing timer |
| Direct spells | DPS × cast_time | Instant = 1.0s (GCD equivalent) |
| DoT ticks | DPS × 0.30 | Reduced - stacks with auto-attacks |
| Direct heals | HPS × cast_time | Same formula as damage spells |
| HoT ticks | HPS × 0.30 | Reduced - stacks with direct heals |
| Absorb shields | HPS × 1.0s | Power Word: Shield, Ice Barrier, etc. |
| Damage shields | DPS × 0.30 | Thorns, Fire Shield, etc. (passive) |

- **Level brackets** - DPS/HPS scales smoothly from level 1-80
- **Gear detection** - Compares melee AP, ranged AP, spell power - picks the best stat
- **Hybrid support** - Ret Paladin uses melee AP, Holy uses spell power automatically
- **Mana scaling** - Spell costs scale by owner level (3-8% of guardian max mana)
- **Immunity throttling** - Divine Shield-type spells have 60s minimum cooldown, only trigger when taking damage
- **Cooldown persistence** - Dismissing and re-summoning does NOT reset spell cooldowns!

**Combat Fairness:**
- Guardians keep ALL auras after combat (both buffs AND debuffs)
- If your guardian got debuffed, it stays - no cheap resets!
- Only Soul Keeper guardians have this behavior (not Hunter/Warlock pets)

#### Soul Keeper Commands
```
.soul absorb         - Capture target (DEAD creatures only)
.soul summon [index] - Open menu or summon by index
.soul dismiss        - Dismiss active guardian
.soul return         - Recall guardian (same as dismiss)
.soul rename <name>  - Rename currently summoned guardian
.soul search <text>  - Search souls by partial name
```

#### ⚡ Performance Optimizations

**Latest:** Fixed critical performance regression with 100+ NPCBots active — custom module hooks now filter bot entities early, preserving full functionality for real players while eliminating unnecessary processing overhead. Soul Keeper's `OnUnitUpdate` reordered for cheap field-read checks before expensive global map lookups. `OnCreatureRemoveWorld` now skips non-player-owned creatures (~95% of despawns). Random enchants `appliedCount` double-count bug fixed.

#### Solo Sustain Features
- **Passive leech** - No need to cast spells, just deal damage!
- **Class-specific** - Melee gets more sustain (face-tanking), ranged less (can kite)
- **Pet support** - Hunter pets and Warlock demons leech too!
- **Combat log visible** - Shows as "Vampiric Embrace" (heal) and "Replenishment" (mana)
- **Fully configurable** - Tune percentages per class in `solo_sustain.conf`
- **Standard client compatible** - No custom spells needed, works with any 3.3.5a client

#### Custom Ramvaris Module
All of Ramvaris' private server Lua customizations ported to C++ for performance. Global toggle ON, all sub-features OFF by default — safe dead module for cloners.

**Features:**
- **Human Stoneform** (Spell 81013) — Custom racial cleanse removing Poison, Disease, Curse, Magic (2min CD)
- **NPC Summon Flavor Text** — 8 custom NPCs with 12 random witty sayings each when summoned
- **Lilly Helper NPC** — Full gossip: city/dungeon/POI teleports, instance reset, talent point purchase
- **Login Spell Grants** — Auto-learn utility spells, profession dual/triple spec unlock, racial fixes. Unlearns 81002/81003/81008 (replaced by enchant speed/fly system)
- **Dark Azeroth Weather** — Biome-aware bad weather: storms in deserts, snow in tundra, rain elsewhere
- **`.special`** — Gossip menu to summon custom NPCs, open bank, open mailbox
- **`.guardianscale`** — Double Soul Keeper guardian visual scale each use (caps at 5x)
- **`.petscale`** — Double any pet visual scale each use (Hunter/Warlock/DK, caps at 5x)
- **Pet Stay on Mount** — Core patch: pets stay summoned when mounting up instead of being dismissed
- **Lottery Enchants** — Diablo-style random enchantments on ALL acquired weapons/armor (any quality, grey through legendary). DB-based, up to 7 enchants per item, level-scaled stats. Vendor purchases only roll on Green+ items (grey/white vendor trash skipped to prevent farming). Non-vendor sources (loot, craft, quest, mail) roll on ANY quality. No quality-based tier gating — the percentile system handles balance naturally. Includes **Movespeed** (1-25% per roll, stacks to +100% = 200% base, affects run+swim+flight, **skipped when mounted** — mounts always use vanilla 100% base) and **Mobility Boost** (1% chance, 3 stages: 100%/200%/300%, combined with speed bonus up to 600% — full superman mode). **Two modes**: Full Flight (level 60+) or Double Jump (level <60 or `.flylock` — press space mid-air for one extra jump, then fall normally). Speed bonus is multiplicative with aura buffs. Gossip-based `.reroll` with Take/Keep preview (500g). Gossip-based `.enchants` viewer with item drill-down. `.flylock` switches between flight and double jump. BG flag auto-drops when airborne.
- **Smart Wandering Bots** — Zone-aware dynamic bot spawning instead of global. Spawns MinAmount–MaxAmount bots ONLY in the player's current zone, despawns on zone change. Optional faction balancing to even out Alliance/Horde ratio. Core patch extends BotDataMgr with zone-specific spawn/despawn API. 30s cooldown prevents zone-border flapping. Per-player tracking for multi-player support. The performance fix for servers with NPCBots.

**Config:**
```ini
CustomRamvaris.Enable = 1                    # Master switch (ON, but all features OFF by default)
CustomRamvaris.DarkAzeroth.Enable = 0        # Example: no custom DB needed
CustomRamvaris.PetStayOnMount.Enable = 0     # Core patch: pets run alongside mounted player
CustomRamvaris.LotteryEnchants.Enable = 0    # Diablo-style enchants + speed/fly/gossip (vendor buys excluded)
CustomRamvaris.LotteryEnchants.MaxSlots = 7  # 1-7 enchants per item (cascading chance)
CustomRamvaris.SmartWanderingBots.Enable = 0 # Zone-aware bot spawning (set NpcBot.WanderingBots.Continents.Count=0!)
CustomRamvaris.SmartWanderingBots.MinAmount = 5
CustomRamvaris.SmartWanderingBots.MaxAmount = 15
CustomRamvaris.SmartWanderingBots.ZoneChangeCooldown = 30
CustomRamvaris.SmartWanderingBots.BalancedFaction.Enable = 0  # Even out Alliance/Horde ratio
```

### 🌿 Database Tweaks

| Tweak | Description |
|-------|-------------|
| **Fast Resource Respawns** | Mining nodes and herbs respawn in 2 minutes instead of 45min-7days. Applied automatically on DB setup. |

> **Note:** The fast respawn SQL is prefixed with `9999_99_99_` to ensure it runs LAST after all other database updates, preventing overwrites.

## Introduction

AzerothCore is an open-source game server application and framework designed for hosting massively multiplayer online role-playing games (MMORPGs). It is based on the popular MMORPG World of Warcraft (WoW) and seeks to recreate the gameplay experience of the original game from patch 3.3.5a.

The original code is based on MaNGOS, TrinityCore, and SunwellCore and has since then had extensive development to improve stability, in-game mechanics, and modularity to the game. AC has also grown into a community-driven project with a significant number of contributors and developers. It is written in C++ and provides a solid foundation for creating private servers that mimic the mechanics and behavior of the official WoW servers.

[NPCBots](https://github.com/trickerer/Trinity-Bots) is AzerothCore mod.


## Installation

Installation instructions are available [here](http://www.azerothcore.org/wiki/Installation).

NPCBots installation guide is available in the [NPCBots Readme](https://github.com/trickerer/Trinity-Bots#npcbot-mod-installation).


## Support

AzerothCore self-made wiki probably has a lot of answers for you.

For help requests, it is recommended to ask your question on [StackOverflow](https://stackoverflow.com/questions/tagged/azerothcore) and link it in [our chat](https://discordapp.com/channels/217589275766685707/284406375495368704).


## Reporting issues

NPCBots issues can be reported via the [Github issue tracker](https://github.com/trickerer/Trinity-Bots/issues/).

Please take the time to review existing issues before submitting your own to
prevent duplicates.


## Submitting fixes

C++ fixes are submitted as [pull requests](https://github.com/trickerer/Azerothcore-wotlk-with-NPCBots/pulls).


You can check the [authors](https://github.com/azerothcore/azerothcore-wotlk/blob/master/AUTHORS) file for more details.

## Important Links

- [NPCBots Readme](https://github.com/trickerer/Trinity-Bots/)

- [Website](http://www.azerothcore.org/)
- [AzerothCore catalogue](http://www.azerothcore.org/catalogue.html  "Modules, tools, and other stuff for AzerothCore") (modules, tools, etc...)
- [Our Discord server](https://discord.gg/gkt4y2x)
- [Our wiki](http://www.azerothcore.org/wiki "Easy to use and developed by AzerothCore founder")
- [Our forum](https://github.com/azerothcore/azerothcore-wotlk/discussions/)
- [Our Facebook page](https://www.facebook.com/AzerothCore/)
- [Our LinkedIn page](https://www.linkedin.com/company/azerothcore/)

## License

- The AzerothCore source code is released under the [GNU GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

It's important to note that AzerothCore is not an official Blizzard Entertainment product, and it is not affiliated with or endorsed by World of Warcraft or Blizzard Entertainment. AzerothCore does not in any case sponsor nor support illegal public servers. If you use this project to run an illegal public server and not for testing and learning it is your own personal choice.

## Special thanks

It's important to note that AzerothCore is not an official Blizzard Entertainment product, and it is not affiliated with or endorsed by World of Warcraft or Blizzard Entertainment. AzerothCore does not in any case sponsor nor support illegal public servers. If you use this project to run an illegal public server and not for testing and learning it is your own personal choice.

[![JetBrains logo.](https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg)](https://jb.gg/OpenSourceSupport)
