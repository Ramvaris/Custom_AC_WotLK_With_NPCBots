# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:HUNTER_RANGED_MODE|Context:AzerothCore_WotLK+NPCBots|Mood:SATISFIED|Goal:TESTING
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

LATEST_FEATURE:
  MULTI_FIX_BATCH:
  - REVERTED: Spell.cpp min_range changes (as Ramires ordered)
  - FIXED: Pick Pocket for non-Rogues (removed CLASS_ROGUE checks in LootHandler.cpp)
  - FIXED: Guardian buff spam in CombatAI AND CasterAI using AITARGET_SELF
  - FIXED: Guardian buff spam in SmartAI too! (SmartScript.cpp SMART_ACTION_CAST)
  - SmartAI creatures (like Enraged Ravager) now skip self-buffs they already have
  - Status: REBUILDING 🍥

SCALING_FORMULAS:
  BEST_RATIO_DESIGN:  Pick max of (meleeAP/expectedMelee, rangedAP/expectedRanged, maxSP/expectedSpell)
  ALL_DAMAGE:  targetDPS × gearRatio (ONE ratio for all attack types)
  OFFHAND: 50% of mainhand DPS
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "The attack swing opcode was the key - it forced true for meleeAttack."
  - "Hunters with projectile weapons now stay in ranged mode."
  - "Auto Shot is spell ID 75, tracked via CURRENT_AUTOREPEAT_SPELL."
  - "The client still sends the attack command, we just tell the server NOT to do melee swings."

ACTIVE_WORK:
  CLEANUP_AND_COMMIT:
  - Removed debug file logging
  - Ready to push to git
  - Status: COMMITTING 🍥

