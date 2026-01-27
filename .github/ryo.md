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
  CLIENT_SCALING_LIMITATION_RESEARCH:
  - Researched TrinityCore #24551 (open since May 2020)
  - Creatures with family > 0 AND pet-type models get CLIENT-scaled
  - This is hardcoded in WoW.exe - ignores server scale values
  - Moths = family 37 (CREATURE_FAMILY_MOTH) → shrinks
  - Blood Elf Scouts = family 0 → stays correct size
  - Removed useless server-side workarounds (100ms delay, etc.)
  - Honestly documented in README with TrinityCore link
  - Status: ACCEPTED AS CLIENT LIMITATION 🍥

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
  - "CLIENT also scales creatures with pet families. TrinityCore #24551."
  - "Open issue since 2020. Nobody fixed it because you CAN'T. Client-side."
  - "Moths shrink, Blood Elf Scouts don't. Family > 0 vs Family = 0."
  - "Some things are just baked into the 3.3.5a client. Accept and move on."

ACTIVE_WORK:
  CLIENT_SCALING_LIMITATION_DOCUMENTED:
  - TrinityCore #24551: Creatures with family > 0 get scaled by CLIENT
  - Models associated with hunter pets trigger client-side scaling
  - Server sends correct scale, client overrides it visually
  - Moths (family=37) shrink, Humanoids (family=0) don't
  - This is HARDCODED in the 3.3.5a client - NO SERVER FIX POSSIBLE
  - Documented in README.md with link to TrinityCore issue
  - Status: DOCUMENTED AS KNOWN LIMITATION 🍥

