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
  [x] - Batch 5: Double jump apex fix, GUID tag removal, gossip cleanup 🍥
  [x] - Batch 6: Double jump complete rewrite — state machine + MovementHandlerScript 🍥

LATEST_FEATURE:
  DOUBLE_JUMP_REWRITE:
  - TASK: Complete rewrite of double jump — proper platformer-style using core opcodes
  - PROBLEM: Previous approach polled movement flags in OnPlayerUpdate for both trigger
    AND landing detection. At knockback apex, flags briefly clear → false landing →
    instant reset → infinite re-trigger → player kicked to sky endlessly.
    Even the s_doubleJumpFellAfter "falling phase" fix was fragile.
  - SOLUTION: State machine + MovementHandlerScript for opcode-driven detection
  - CHANGES:
    1. STATE_MACHINE: DoubleJumpState enum (DJ_GROUNDED / DJ_AIRBORNE / DJ_USED)
       with s_djState and s_djJumpTime tracking maps. Replaced s_doubleJumpUsed
       and s_doubleJumpFellAfter entirely.
    2. MOVEMENT_SCRIPT: New LotteryEnchants_MovementScript (MovementHandlerScript)
       hooks into OnPlayerMove for precise opcode detection:
       - MSG_MOVE_JUMP (0x0BB): GROUNDED → AIRBORNE, records timestamp
       - MSG_MOVE_FALL_LAND (0x0C9): USED → GROUNDED (re-enable SetCanFly),
         or AIRBORNE → GROUNDED (unused double jump, just reset)
    3. ON_PLAYER_UPDATE: Reduced to ONLY handling DJ_AIRBORNE → DJ_USED transition.
       Checks IsFlying() (space pressed mid-air with CAN_FLY), enforces 300ms delay,
       applies KnockbackFrom(pos, 0, 7.96f) for vanilla jump velocity, then
       SetCanFly(false) + state = DJ_USED.
    4. CONSTANTS: DOUBLE_JUMP_MIN_DELAY_MS=300, DOUBLE_JUMP_Z_SPEED=7.96f
       (vanilla WoW jump z-velocity, not arbitrary values)
  - ARCHITECTURE: Split responsibilities cleanly:
    * MovementHandlerScript: Precise jump/landing detection via client opcodes
    * OnPlayerUpdate: Mid-air trigger only (IsFlying + delay check + knockback)
    * No movement flag polling for landing — MSG_MOVE_FALL_LAND is definitive
  - FILES_CHANGED:
    * random_enchants.cpp: Complete double jump rewrite (~100 lines changed)
    * README.md: Updated double jump description with state machine details
  - BUILD: CLEAN 🍥
  - STATUS: SHIPPED 🍥

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

