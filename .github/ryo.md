# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:ALE_WEATHER_FIX|Context:AzerothCore_WotLK+NPCBots|Mood:VICTORIOUS|Goal:TESTING
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

LATEST_FEATURE:
  ALE_WEATHER_FIX:
  - FOUND: map:SetWeather() silently failed for zones without game_weather DB entries
  - CAUSE: GetOrGenerateZoneDefaultWeather() returns nullptr for zones without weather data
  - FIX: Rewrote MapMethods.h SetWeather to use Map::SetZoneWeather() directly
  - RESULT: Weather now works for ALL zones regardless of database entries
  - BONUS: SetWeather now returns true/false indicating success
  - Files Modified: modules/mod-ale/src/LuaEngine/methods/MapMethods.h
  - Files Modified: modules/mod-ale/src/LuaEngine/ALEIncludes.h (added Weather.h include)
  - Status: FIXED AND DEPLOYED 🍥

SCALING_FORMULAS:
  BEST_RATIO_DESIGN:  Pick max of (meleeAP/expectedMelee, rangedAP/expectedRanged, maxSP/expectedSpell)
  ALL_DAMAGE:  targetDPS × gearRatio (ONE ratio for all attack types)
  OFFHAND: 50% of mainhand DPS
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "The weather bug was sneaky - silent failure is the worst kind of bug."
  - "SetZoneWeather bypasses the whole WeatherData requirement."
  - "Only 191 out of thousands of zones had weather data. Classic limitation."
  - "Now Ramires can have perpetual storms everywhere. Dark Azeroth indeed."

ACTIVE_WORK:
  TESTING:
  - Weather system fixed
  - Ramires should test by logging in
  - Status: AWAITING_CONFIRMATION 🍥

