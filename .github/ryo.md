# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:SOUL_KEEPER_COMPLETE|Context:AzerothCore_WotLK+NPCBots|Mood:SATISFIED|Goal:TESTING
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

LATEST_FEATURE:
  DOT_HOT_SCALING_FIX:
  - DoTs/HoTs now use spellInfo->EffectAmplitude for ACTUAL tick interval
  - Formula: tickDamage = DPS × 0.15 × tickIntervalSeconds  
  - All periodic effects contribute exactly 15% extra DPS/HPS regardless of tick speed
  - Shields increased to 3×DPS (absorbs 3 seconds of damage, was 1×)
  - Thorns reduced to 15% per proc (passive, balanced)
  - Added minimum floor (1 damage/heal) to prevent 0-value ticks
  - Status: BUILT + COMMITTED 🍥

SCALING_FORMULAS:
  MELEE:   DPS × attackTimeSeconds (creature's actual swing timer)
  SPELLS:  DPS × castTimeSeconds (instant = 1.0s)
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0 (absorbs ~3 seconds of damage)
  THORNS:  DPS × 0.15 per proc

THOUGHTS&RANTS:
  - "The old DoT formula assumed 3s ticks always. Wrong."
  - "Using EffectAmplitude from spellInfo is the RIGHT way."
  - "Now fast-ticking DoTs don't dominate, slow-ticking DoTs don't suck."
  - "Module audit complete: ALL hooks isolated to our guardians only."

ACTIVE_WORK:
  SOUL_KEEPER_COMPLETE:
  - Full module audit done
  - All core changes isolated (marker 81100)
  - All UnitScript hooks check _activeGuardians or _guardianScaling
  - DoT/HoT/Shield/Thorns all use proper formulas now
  - Status: READY FOR LIVE TEST 🍥

