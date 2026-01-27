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
  [x] - Guardian Visual Scale Fix (SetObjectScale) 🍥

LATEST_FEATURE:
  LANGUAGE_COMPREHENSION_FIX:
  - WoW 3.3.5a client does ALL scrambling (based on race only)
  - Client ignores learned skills and SPELL_AURA_COMPREHEND_LANGUAGE
  - Server sends plain text + language ID to ALL players
  - Only fix: send LANG_UNIVERSAL to players who know the language
  - Trade-off: Player sees clean text but doesn't know original language
  - NO PREFIX - clean text, maximum immersion
  - Status: BUILT + READY FOR COMMIT 🍥

SCALING_FORMULAS:
  MELEE:   DPS × attackTimeSeconds (creature's actual swing timer)
  SPELLS:  DPS × castTimeSeconds (instant = 1.0s)
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0 (absorbs ~3 seconds of damage)
  THORNS:  DPS × 0.15 per proc

THOUGHTS&RANTS:
  - "WoW 3.3.5a client design... Ramires was right to question it."
  - "Why does the SERVER send plain text only for CLIENT to scramble?"
  - "Client ignores SPELL_AURA_COMPREHEND_LANGUAGE. Who coded that?"
  - "Only fix: send LANG_UNIVERSAL for understood languages."
  - "No prefix - keeps immersion. You just won't know WHICH language."
  - "This is the best we can do without modifying the client."

ACTIVE_WORK:
  LANGUAGE_COMPREHENSION_FIX:
  - WoW 3.3.5a client does ALL scrambling (based on race only)
  - Client ignores learned skills and SPELL_AURA_COMPREHEND_LANGUAGE
  - Server sends plain text + language ID to ALL players
  - Only fix: send LANG_UNIVERSAL to players who know the language
  - Trade-off: Player sees clean text but doesn't know original language
  - Removed ugly "[LanguageName] " prefix - now clean text only
  - Status: BUILT + READY FOR COMMIT 🍥

