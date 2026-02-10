# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:SOUL_KEEPER_FIXES|Context:AzerothCore_WotLK+NPCBots|Mood:FOCUSED|Goal:TESTING
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

LATEST_FEATURE:
  SOUL_KEEPER_RANGED_FIX:
  - TASK: Fix ranged guardians doing nearly zero damage
  - ROOT_CAUSES:
    * Shoot spells used castTime normalization (instant=1.0s → 50% of melee per hit)
    * Guardian::UpdateAttackPowerAndDamage added STR-based AP contamination to melee
    * Missing weapon damage spell guards in ModifySpellDamageTaken hook
  - FIX:
    * Attack-speed normalization: max(castTime, attackSpeed) for spell damage
    * STR=10 (AP=0) to prevent unintended melee bonus from creature template
    * Skip WEAPON_DAMAGE/NORMALIZED_WEAPON_DMG spell effects in hook
    * Added offhand display fields, baseAttackTimeMs to scaling info
  - AUDIT: All non-percentual damage/heal/shield scaling verified comprehensive
  - BUILD: SUCCESS ✓
  - STATUS: PUSHED 🍥

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
  - "NPCBots audit complete. 54 Cell::VisitObjects in bot_ai.cpp ALONE. 200yd grid searches for Sindragosa. O(n²) BuffAndHealGroup. All by-design, all upstream — touching it = merge hell. The Blademaster mirror image crash is a raw-pointer-in-BasicEvent nightmare with set-modification-during-iteration in UnsummonAll. Warlock Life Tap can self-kill via uncapped ModifyHealth(-damage). Both upstream, both risky to fix. Ramires already disabled BM — smart move."

ACTIVE_WORK:
  - None 🍥

NPCBOTS_AUDIT_FINDINGS:
  UPSTREAM_ISSUES_DO_NOT_TOUCH:
  - Blademaster Mirror Image: raw Creature* in BasicEvents + set modification during UnsummonAll iteration → crashes (DISABLED by Ramires)
  - Warlock Life Tap: uncapped ModifyHealth(-damage) can self-kill → "dead while casting" state
  - bot_ai.cpp: 54× Cell::VisitObjects, 200yd Sindragosa search, O(n²) BuffAndHealGroup — all by-design
  - botmgr.cpp: Actually well-structured, CalculateAoeSpots runs once per player
  - botwanderful.cpp: Just waypoint graph data, runtime wandering is in botdatamgr
  - VERDICT: Performance hogs are inherent to NPCBots design (grid searches scale with bot count). No safe fixes without risking upstream merge conflicts.

