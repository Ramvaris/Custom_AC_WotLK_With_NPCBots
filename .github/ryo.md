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
  [x] - Batch 7: Nuke double jump, simplify to mount-style flight + .flylock toggle 🍥
  [x] - Lottery Enchants: Flat 1-77 value refactor — replaced DBC pool lookups with synthetic enchant IDs 🍥

LATEST_FEATURE:
  FLIGHT_SIMPLIFICATION:
  - TASK: Remove broken double jump, simplify to mount-style flying
  - PROBLEM: Double jump was fundamentally unfixable in 3.3.5a. Two full rewrites
    (knockback + flag polling, then state machine + MovementHandlerScript) both failed.
    The knockback approach had infinite sky-kick. The state machine approach made the
    player just fly instead of double-jumping. Flying mount behavior already exists
    in the engine and works perfectly — no need to reinvent it.
  - SOLUTION: Nuke everything. Just use SetCanFly(true) → flying mount behavior.
    Jump on ground, press space mid-air to fly. That's it.
  - CHANGES:
    1. REMOVED: DoubleJumpState enum, s_djState, s_djJumpTime maps, DOUBLE_JUMP_MIN_DELAY_MS,
       DOUBLE_JUMP_Z_SPEED constants, entire LotteryEnchants_MovementScript class,
       OnPlayerUpdate double jump detection, #include GameTime.h, #include Opcodes.h
    2. SIMPLIFIED: RecalcLotterySpeedAndFly — two branches: fullFlight → SetCanFly(true),
       else → SetCanFly(false). No more doubleJump variable or state checks.
    3. SIMPLIFIED: HasLotteryFullFlight() — removed GetLevel() >= 60 check.
       Any player with Mobility Boost can fly. No level restriction.
    4. SIMPLIFIED: .flylock command — just "Flight DISABLED" / "Flight ENABLED".
       No more double jump mode messages.
    5. SIMPLIFIED: OnPlayerUpdate — only BG flag enforcement remains.
  - NET RESULT: ~130 lines deleted. Flying works like flying mounts (because it IS
    flying mount behavior). .flylock toggles it off if you want to stay grounded.
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
  - "Three rounds of debugging to find a silent integer math edge case. CalculatePct(1, 15) = 0. Every bracket gets zero bots. The remainder loop finds nothing to fix. brackets_shuffled is empty. Zero bots spawn. The function returns TRUE. No error, no crash, no log. Just... nothing. Upstream code assumes Count >= ~10. With Count=1, the entire percentage-based bracket distribution collapses silently. And the ASSERT(!level_nodes.empty()) would've been a latent crash bomb if we ever DID fix the first bug — spawning bracket 7 (70-79) bots in Elwynn (1-10) would nuke the server. Two bugs stacked: the first prevented the second from ever triggering. Poetic, in a terrible way. Dattebayo."
  - "Fixed the Warlock Life Tap self-kill. 5 lines of code to prevent a game-breaking bug. Cap health cost, check IsAlive after, bail if dead. The upstream code just raw ModifyHealth(-damage) with zero guards. And then AzerothCore has 6+ ABORT() calls in hot paths like aura removal and spell cleanup that crash the ENTIRE server for recoverable states. Replaced them all with LOG_ERROR + graceful recovery. Students, I swear."
  - "Ported 1066 lines of Lua to C++. Found TWO registration bugs in the original Lua that meant half the features NEVER WORKED. commands.lua had a CommandHandlerFunction that was never RegisterPlayerEvent'd. lilly.lua registered GossipHello but forgot GossipSelect — so you could open the menu but never click anything. Ramires was running broken Lua for who knows how long. Now it's all clean C++ with proper hooks. Dattebayo."
  - "210% CPU with ONE PLAYER logged in. EIGHT map update threads for a solo server. MinWorldUpdateTime=1 letting the world loop spin at 1000 Hz like it's trying to render frames for a VR headset. 120 wandering bots keeping hundreds of grids loaded across 4 continents. The pet revive delay? Architectural — AzerothCore fires 4 serial SELECT queries on a SINGLE DB connection, then polls for completion on the NEXT world tick. You literally can't fix it without rewriting the async query pipeline. At least the config tuning should cut CPU by 50%+. Dattebayo."
  - "Five bugs, five root causes, zero in common. Transmog NPC invisible because CanBeSeen checks GetOwner() but TEMPSUMMON_TIMED_DESPAWN doesn't set UNIT_FIELD_SUMMONEDBY. Reagent bank lag because Execute() is fire-and-forget — the menu refresh query beats the write. Chat format broken because PSendSysMessage uses fmt::format but I wrote printf. Stat stacking because OnPlayerUnequip doesn't fire on swaps. Speed not level-scaled by design choice but should be for balance. Every single one is a different category of mistake. I'm learning five lessons at once. Dattebayo."

ACTIVE_WORK:
  LOTTERY_ENCHANT_FLAT_ROLL_REFACTOR:
  - TASK: Replace DBC pool-based value selection with flat 1-77 rolls
  - PROBLEM: When stat TYPE was selected, VALUE was determined by which DBC entry
    was randomly picked from the pool. Most DBC entries have low values, making
    high rolls astronomically rare ON TOP of already-rare slot counts.
  - SOLUTION: Scrap DBC pools entirely. Synthetic enchant ID encoding:
    - Stat:   800000 + (ITEM_MOD_* × 100) + value  (range 800001..804777)
    - Resist: 850000 + (school × 100) + value      (range 850101..850677)
    - Speed:  900001 + value                         (range 900002..900078)
    - Fly:    900100 + stage                          (unchanged 900101..900103)
    Flat pool of 20 types (13 stats + 6 resists + 1 speed), equal weight.
    urand(1, 77) for every stat value. 77 = thematic (7 slots × lucky 7s).
    Speed expanded from 1-25% to 1-77%, NOT level-scaled (always full value).
    7 color tiers: each 11 values (1-11=Grey, 12-22=White, ..., 67-77=Red).
  - CHANGES:
    1. DELETED: BuildEnchantPoolsFromDBC(), s_enchantPools, s_globalMaxEnchantValue,
       s_poolsBuilt, AllowedStat enum, IsAllowedStatType(), IsAllowedResistSchool()
    2. ADDED: LOTTERY_MAX_VALUE=77, LOTTERY_STAT_POOL[] array, synthetic ID constants,
       IsCustomStatEnchant(), IsCustomResistEnchant(), decoder functions
    3. REWROTE: GetRandomEnchant (flat pool + urand), ApplyLotteryEnchantStat (synthetic
       ID decoding + DBC fallback), FormatEnchantLine (synthetic display), GetTierFromValue
    4. UPDATED: ShowEnchantsSummary, RecalcLotterySpeedAndFly (speed NOT level-scaled),
       startup hook (removed pool build call), RollNewEnchants comment, header doc
  - BACKWARD_COMPAT: Existing items with old DBC enchant IDs still work via fallback
    paths in Apply, Format, and Summary functions. No DB migration needed.
  - MIGRATION: tools/migrate_lottery_enchants.py re-rolls legacy DB rows into synthetic format.
  - BUILD: CLEAN 🍥
  - STATUS: SHIPPED 🍥

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

