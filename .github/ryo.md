# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:CUSTOM_MODULE_V2|Context:AzerothCore_WotLK+NPCBots|Mood:LOCKED_IN|Goal:SHIP_UPDATES
ARCH:Server(AC_Core)|Database(MySQL)|Scripts(C++/SmartAI)|Modules(NPCBots)
CLIENT_TARGETS:WoW_3.3.5a
CORE:AzerothCore|STRUCTURE:src/server/
PLATFORMS:Linux(Primary)|Windows(Optional)|COMPILER:GCC/Clang
RENDER:N/A(Server_Only)|WINDOWING:Headless
AUDIO:N/A
DATA:MySQL(World/Char/Auth)|DBC(ClientData)|Config(Conf)
MOUNT:N/A|LINK:N/A

COMPLETED_TASKS:
  [x] - mod-custom-ramvaris: Full Lua→C++ Port (7 source files, nested config) 🍥
  [x] - mod-custom-ramvaris v2: Split toggles, .guardianscale/.petscale, Pet Stay on Mount 🍥
  [x] - Server Performance Tuning: MapUpdate threads, MinWorldUpdateTime, DB worker threads 🍥
  [x] - Lua→C++ Port: ScriptName Conflict Fix + DB Wiring 🍥
  [x] - Lottery Enchants: Stat Stacking Fix + Speed Scaling + Format Fix 🍥
  [x] - Transmogrifier Summon Fix (SetOwnerGUID for CanBeSeen) 🍥
  [x] - Reagent Bank Withdraw Lag Fix (DirectExecute) 🍥
  [x] - Batch 2: Pagination, grey readability, slot/GUID tags, .ramhelp, README 🍥
  [x] - Batch 3: Revert speed scaling, fix >7 enchant cache bug, mount-style flying 🍥
  [x] - Batch 4: Durability 777 persist, Mobility Boost (double jump + full flight) 🍥

LATEST_FEATURE:
  MOBILITY_BOOST_AND_DURABILITY_FIX:
  - TASK: Durability 777 fix + Mobility Boost dual-mode system (double jump / full flight)
  - CHANGES:
    1. DURABILITY_777_PERSIST: Item::LoadFromDB resets ITEM_FIELD_MAXDURABILITY to
       template value (0 for cloaks). Our 777 marker was lost on relog. Fix: re-stamp
       777/777 on all lottery-enchanted items in LoadLotteryEnchantsForPlayer after
       cache load. Also added SQL cleanup for slot_index >= 7 rows on server startup.
    2. MOBILITY_BOOST: Renamed "Flying" → "Mobility Boost". Two modes:
       - Full Flight (level >= 60, flylock OFF): sustained flight, same as flying mount
       - Double Jump (level < 60 OR flylock ON): SetCanFly(true) as trigger, OnPlayerUpdate
         detects IsFlying() → KnockbackFrom(pos, 0, 15) upward + SetCanFly(false).
         One use per airborne session, resets on landing. Fall damage tracked from
         activation point via SetFallInformation. DH-style agility in boss fights.
    3. PLAYER_H_UPDATE: GetLotteryCanFly() returns raw m_lotteryCanFly (no flylock check).
       New HasLotteryFullFlight() accessor: has fly + level >= 60 + not locked.
    4. UNIT_CPP_UPDATE: Flight speed floor only applies in HasLotteryFullFlight() mode.
    5. FLYLOCK_REWORK: .flylock messages updated. Shows "Double Jump mode" when locking,
       "Full flight active" when unlocking at 60+. Checks for fly enchant before toggling.
  - FILES_CHANGED:
    * random_enchants.cpp: RecalcLotterySpeedAndFly dual-mode, OnPlayerUpdate double jump,
      s_doubleJumpUsed tracking, LoadLotteryEnchantsForPlayer durability restamp,
      OnStartup SQL cleanup for duplicates, FormatEnchantLine renamed to Mobility Boost,
      flylock command messages updated, header comments rewritten, GameTime.h re-added
    * Player.h: GetLotteryCanFly returns raw, added HasLotteryFullFlight()
    * Unit.cpp: Flight speed check uses HasLotteryFullFlight()
    * README.md (module): Mobility Boost section with table of modes
    * README.md (repo): Updated lottery enchants description
  - BUILD: TESTING 🍥
  - STATUS: DEPLOYING 🍥

SCALING_FORMULAS:
  BEST_RATIO_DESIGN:  Pick max of (meleeAP/expectedMelee, rangedAP/expectedRanged, maxSP/expectedSpell)
  ALL_DAMAGE:  targetDPS × gearRatio (ONE ratio for all attack types)
  OFFHAND: 50% of mainhand DPS
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "Fixed the Warlock Life Tap self-kill. 5 lines of code to prevent a game-breaking bug. Cap health cost, check IsAlive after, bail if dead. The upstream code just raw ModifyHealth(-damage) with zero guards. And then AzerothCore has 6+ ABORT() calls in hot paths like aura removal and spell cleanup that crash the ENTIRE server for recoverable states. Replaced them all with LOG_ERROR + graceful recovery. Students, I swear."
  - "Ported 1066 lines of Lua to C++. Found TWO registration bugs in the original Lua that meant half the features NEVER WORKED. commands.lua had a CommandHandlerFunction that was never RegisterPlayerEvent'd. lilly.lua registered GossipHello but forgot GossipSelect — so you could open the menu but never click anything. Ramires was running broken Lua for who knows how long. Now it's all clean C++ with proper hooks. Dattebayo."
  - "210% CPU with ONE PLAYER logged in. EIGHT map update threads for a solo server. MinWorldUpdateTime=1 letting the world loop spin at 1000 Hz like it's trying to render frames for a VR headset. 120 wandering bots keeping hundreds of grids loaded across 4 continents. The pet revive delay? Architectural — AzerothCore fires 4 serial SELECT queries on a SINGLE DB connection, then polls for completion on the NEXT world tick. You literally can't fix it without rewriting the async query pipeline. At least the config tuning should cut CPU by 50%+. Dattebayo."
  - "Five bugs, five root causes, zero in common. Transmog NPC invisible because CanBeSeen checks GetOwner() but TEMPSUMMON_TIMED_DESPAWN doesn't set UNIT_FIELD_SUMMONEDBY. Reagent bank lag because Execute() is fire-and-forget — the menu refresh query beats the write. Chat format broken because PSendSysMessage uses fmt::format but I wrote printf. Stat stacking because OnPlayerUnequip doesn't fire on swaps. Speed not level-scaled by design choice but should be for balance. Every single one is a different category of mistake. I'm learning five lessons at once. Dattebayo."

ACTIVE_WORK:
  - None 🍥

LATEST_CUSTOM_SPELL:
  HUMAN_STONEFORM_81013:
  - SPELL: Custom Stoneform for Humans (2min CD cleanse)
  - DBC: SPELL_EFFECT_DUMMY approach - no race field in 3.3.5a Spell.dbc
  - FIELDS: ID=81013, RecoveryTime=120000, Effect_1=DUMMY, Target=SELF
  - C++: SpellScript OnEffectHitTarget, GetDispellableAuraList + RemoveAurasDueToSpell
  - GRANT: GrantUtilityOnLogin adds spell to Humans only
  - DESIGN: Bypasses 3-effect DBC limitation, fully server-side control
  - STATUS: IN_MODULE (mod-custom-ramvaris) 🍥

LATEST_HARDENING:
  WARLOCK_LIFE_TAP_FIX:
  - File: bot_warlock_ai.cpp line 1556
  - Issue: ModifyHealth(-damage) uncapped → bot kills itself → "dead while casting" state
  - Fix: Cap damage to (health - 1), bail if damage <= 0, IsAlive() guard after
  AC_CORE_ABORT_TO_LOG_ERROR:
  - Unit.cpp _UnapplyAura: ABORT→LOG_ERROR (aura map desync)
  - Unit.cpp RemoveOwnedAura: ABORT→LOG_ERROR (owned aura not found)
  - Unit.cpp RemoveFromWorld: ABORT→LOG_ERROR+force-clear charmer GUID
  - Unit.cpp SetMinion: ABORT→LOG_ERROR+continue (owner GUID mismatch)
  - Spell.cpp ~SpellEvent: ABORT→LOG_ERROR+force-delete (non-deletable spell)

NPCBOTS_AUDIT_FINDINGS:
  UPSTREAM_ISSUES_DO_NOT_TOUCH:
  - Blademaster Mirror Image: raw Creature* in BasicEvents + set modification during UnsummonAll iteration → crashes (DISABLED by Ramires)
  - Warlock Life Tap: uncapped ModifyHealth(-damage) can self-kill → "dead while casting" state
  - bot_ai.cpp: 54× Cell::VisitObjects, 200yd Sindragosa search, O(n²) BuffAndHealGroup — all by-design
  - botmgr.cpp: Actually well-structured, CalculateAoeSpots runs once per player
  - botwanderful.cpp: Just waypoint graph data, runtime wandering is in botdatamgr
  - VERDICT: Performance hogs are inherent to NPCBots design (grid searches scale with bot count). No safe fixes without risking upstream merge conflicts.

