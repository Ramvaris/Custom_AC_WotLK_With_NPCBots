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
  [x] - Guardian Damage Scaling Fix (Melee/Ranged separation + Dual-Wield) 🍥

LATEST_FEATURE:
  GUARDIAN_DAMAGE_SCALING_VERIFIED:
  - Best ratio design confirmed working via debug test
  - Stats correctly read: MeleeAP, RangedAP, MaxSP all working
  - Ratios correctly calculated and best one selected
  - Debug output removed, code cleaned up
  - Status: COMPLETE 🍥

SCALING_FORMULAS:
  BEST_RATIO_DESIGN:  Pick max of (meleeAP/expectedMelee, rangedAP/expectedRanged, maxSP/expectedSpell)
  ALL_DAMAGE:  targetDPS × gearRatio (ONE ratio for all attack types)
  OFFHAND: 50% of mainhand DPS
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "Debug test confirmed: stats are being read correctly!"
  - "SpellPower won for the Paladin as expected (169 SP vs 139 AP)"
  - "Best ratio design is working as intended."
  - "Time to clean up and commit."

ACTIVE_WORK:
  CLEANUP_AND_COMMIT:
  - Removed debug file logging
  - Ready to push to git
  - Status: COMMITTING 🍥

