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
  [x] - HUD Initialization 🍥
  [x] - Context Switch to AzerothCore 🍥
  [x] - Persona Synchronization 🍥
  [x] - Soul Keeper Guardian AI v3 🍥
  [x] - Soul Keeper Comprehensive Scaling System 🍥
  [x] - Immunity Buff SELF-ONLY Fix 🍥
  [x] - Player Heal Protection (TYPEID_PLAYER check) 🍥
  [x] - Native AI Buff Spam Prevention 🍥
  [x] - Gossip Menu UX Overhaul 🍥
  [x] - Core CombatAI Buff Spam Fix (GUARDIAN-ONLY) 🍥
  [x] - DoT/HoT Tick Interval Scaling Fix 🍥
  [x] - Guardian Damage Scaling Fix (Melee/Ranged separation + Dual-Wield) 🍥
  [x] - Hunter Ranged-Only Mode (No Melee Auto-Attack with Bow/Gun/XBow) 🍥
  [x] - ALE SetWeather Fix (Works for ALL zones now!) 🍥
  [x] - Cross-Class Skill Support (Pick Pocket for all!) 🍥
  [x] - Upstream Merge (48 files, 1897+/1150-) 🍥
  [x] - Solo Sustain Pet/Guardian Damage as Leech Source 🍥
  [x] - Soul Keeper Ranged Guardian Damage Fix 🍥
  [x] - Module Performance Death Spiral Fix (Bot Filtering) 🍥
  [x] - Warlock Bot Life Tap Self-Kill Fix 🍥
  [x] - AC Core Defensive Hardening (ABORT→LOG_ERROR) 🍥
  [x] - mod-custom-ramvaris: Full Lua→C++ Port (7 source files, nested config) 🍥
  [x] - mod-custom-ramvaris v2: Split toggles, .guardianscale/.petscale, Pet Stay on Mount 🍥
  [x] - Server Performance Tuning: MapUpdate threads, MinWorldUpdateTime, DB worker threads 🍥

LATEST_FEATURE:
  SERVER_PERFORMANCE_TUNING:
  - TASK: Diagnose 210% CPU on worldserver, optimize without breaking functionality
  - DIAGNOSIS:
    * 8 MapUpdate threads at 20-40% CPU each = ~220% CPU combined
    * 120 wandering bots across 4 continents keeping hundreds of grids permanently loaded
    * MinWorldUpdateTime=1 letting world loop tight-spin at up to 1000 Hz
    * Main thread 10%, other 10 threads (DB/network) near 0%
    * Pet revive delay: architectural — async DB round-trip (4 SELECT queries serially)
      when pet corpse despawns after 60s. Fast path (corpse exists) is instant.
  - CHANGES:
    * MinWorldUpdateTime: 1 → 10 (100 Hz still buttery smooth, prevents tight-loop)
    * MapUpdate.Threads: 8 → 2 (solo server, 8 threads was massive overkill)
    * CharacterDatabase.WorkerThreads: 1 → 2 (reduces pet revive DB queue latency)
    * NpcBot.WanderingBots.Continents.Count: kept at 120 per Ramires's choice
  - PET_REVIVE_DELAY_ANALYSIS:
    * Two code paths in EffectResurrectPet (SpellEffects.cpp:5447)
    * Fast path: Pet corpse still exists → instant revive (setDeathState, restore HP)
    * Slow path: Pet corpse despawned → SummonPet → LoadPetFromDB → async DB
      4 SELECT queries (declined_name, aura, spell, spell_cooldown) serially
      Results polled next WorldSession::Update tick → visible 0.5-1s gap
    * NOT fixable without core architecture changes (async query pattern is fundamental)
  - BUILD: N/A (config-only changes, no recompile needed)
  - STATUS: APPLIED 🍥

SCALING_FORMULAS:
  BEST_RATIO_DESIGN:  Pick max of (meleeAP/expectedMelee, rangedAP/expectedRanged, maxSP/expectedSpell)
  ALL_DAMAGE:  targetDPS × gearRatio (ONE ratio for all attack types)
  OFFHAND: 50% of mainhand DPS
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "Guardian::UpdateAttackPowerAndDamage and UpdateDamagePhysical for ranged/offhand are literal no-ops for the Guardian class. They return immediately. Hours wasted thinking those calls did something."
  - "The AP contamination was sneaky. STR-based AP added inconsistent bonus — a Defias Bandit (STR 30) would hit different than a Gnoll Brute (STR 80). Now it's clean: STR=10, AP=0, ALL damage from our formula."
  - "Ranged creatures doing 50% of melee damage because instant Shoot = 1.0s normalization while melee = 2.0s swing. The fix is elegant: max(castTime, attackSpeed). Equal DPS for everyone."
  - "Full scaling audit done. DoTs, HoTs, shields, thorns, melee, ranged, weapon spells — all covered. Non-percentual scaling is comprehensive."
  - "Solo Sustain pet damage fix was simpler — just resolve pet/guardian owner for leech. Same % for all damage sources."
  - "THE PERFORMANCE DEATH SPIRAL: 100 bots × OnDamage hooks × (HealBySpell + guardian iteration + aura checks) = 50,000+ calls/second! Added IsNPCBot() checks — took 5 minutes to fix what could've killed the server."
  - "NPCBots audit complete. 54 Cell::VisitObjects in bot_ai.cpp ALONE. 200yd grid searches for Sindragosa. O(n²) BuffAndHealGroup. All by-design, all upstream — touching it = merge hell. The Blademaster mirror image crash is a raw-pointer-in-BasicEvent nightmare with set-modification-during-iteration in UnsummonAll. Ramires already disabled BM — smart move."
  - "Fixed the Warlock Life Tap self-kill. 5 lines of code to prevent a game-breaking bug. Cap health cost, check IsAlive after, bail if dead. The upstream code just raw ModifyHealth(-damage) with zero guards. And then AzerothCore has 6+ ABORT() calls in hot paths like aura removal and spell cleanup that crash the ENTIRE server for recoverable states. Replaced them all with LOG_ERROR + graceful recovery. Students, I swear."
  - "Ported 1066 lines of Lua to C++. Found TWO registration bugs in the original Lua that meant half the features NEVER WORKED. commands.lua had a CommandHandlerFunction that was never RegisterPlayerEvent'd. lilly.lua registered GossipHello but forgot GossipSelect — so you could open the menu but never click anything. Ramires was running broken Lua for who knows how long. Now it's all clean C++ with proper hooks. Dattebayo."
  - "210% CPU with ONE PLAYER logged in. EIGHT map update threads for a solo server. MinWorldUpdateTime=1 letting the world loop spin at 1000 Hz like it's trying to render frames for a VR headset. 120 wandering bots keeping hundreds of grids loaded across 4 continents. The pet revive delay? Architectural — AzerothCore fires 4 serial SELECT queries on a SINGLE DB connection, then polls for completion on the NEXT world tick. You literally can't fix it without rewriting the async query pipeline. At least the config tuning should cut CPU by 50%+. Dattebayo."

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

