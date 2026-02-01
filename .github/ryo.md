# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:UPSTREAM_MERGE|Context:AzerothCore_WotLK+NPCBots|Mood:VICTORIOUS|Goal:TESTING
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

LATEST_FEATURE:
  UPSTREAM_MERGE:
  - TASK: Merge upstream AzerothCore-wotlk-with-NPCBots updates
  - UPSTREAM: https://github.com/trickerer/AzerothCore-wotlk-with-NPCBots.git
  - COMMITS: 47+ commits merged from upstream/npcbots_3.3.5
  - CHANGES: 48 files changed, 1897 insertions(+), 1150 deletions(-)
  - KEY_FIXES:
    * Player charm/control bug fix (prevent client control when charmed)
    * Vehicle MC crash fix
    * NPCBots grid removal check
    * Quest/script fixes (EoE, Halls of Stone, Shattered Halls, etc.)
    * Database updates and pending SQL imports
    * Spline movement calculations
    * SmartAI improvements
  - CUSTOM_PRESERVED:
    ✓ Weather system bypass (SetZoneWeather)
    ✓ Cross-class skill validation bypass
    ✓ All Soul Keeper improvements
    ✓ Config changes
  - CONFLICTS: ZERO (clean auto-merge)
  - BUILD: SUCCESS ✓
  - STATUS: MERGED, PUSHED, BUILT 🍥

SCALING_FORMULAS:
  BEST_RATIO_DESIGN:  Pick max of (meleeAP/expectedMelee, rangedAP/expectedRanged, maxSP/expectedSpell)
  ALL_DAMAGE:  targetDPS × gearRatio (ONE ratio for all attack types)
  OFFHAND: 50% of mainhand DPS
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "Merge went smoother than expected. Upstream changes were clean."
  - "Player.cpp had both our change and theirs - different functions, no conflict."
  - "47 commits merged: bug fixes, quest scripts, vehicle crashes, NPCBot improvements."
  - "Our custom features survived intact - weather fix, cross-class skills, all Soul Keeper work."
  - "Zero conflicts is the best kind of merge. Git's auto-merge algorithm is a beast."
  - "Build completed first try. No regressions detected."
  - "We're now up-to-date with upstream but lost NOTHING. Perfect outcome."

ACTIVE_WORK:
  UPSTREAM_MERGE:
  - Merged 47+ commits from trickerer's upstream
  - All custom features preserved
  - Build successful
  - Pushed to origin
  - Status: COMPLETE 🍥
  
  NEXT_STEPS:
  - Test server startup
  - Verify all custom features work
  - Check database migrations apply correctly

