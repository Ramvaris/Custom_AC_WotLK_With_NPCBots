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
  DAMAGE_SCALING_FIX:
  - Fixed melee/ranged/spell damage scaling to be INDEPENDENT
  - Melee creatures scale from owner's melee AP ONLY
  - Ranged creatures scale from owner's ranged AP ONLY
  - Added offhand damage scaling for dual-wielders (50% of mainhand)
  - Spell damage uses best of melee/ranged/spell ratio
  - Status: FIXED 🍥

SCALING_FORMULAS:
  MELEE:   meleeDPS = targetDPS × (meleeAP / expectedMelee)
  RANGED:  rangedDPS = targetDPS × (rangedAP / expectedRanged)
  OFFHAND: 50% of mainhand DPS
  SPELLS:  Uses best ratio of melee/ranged/spell for gearRatio
  DOTS:    DPS × 0.15 × tickIntervalSeconds
  HOTS:    HPS × 0.15 × tickIntervalSeconds
  SHIELDS: HPS × 3.0

THOUGHTS&RANTS:
  - "WoW 3.3.5a client design... Ramires was right to question it."
  - "Hunter RAP = high, Hunter melee AP = low. Different stats!"
  - "Can't use 'pick the best stat' for physical damage."
  - "Melee creatures MUST scale from melee AP only."
  - "Ranged creatures MUST scale from ranged AP only."
  - "Dual-wielders need offhand damage set too - was missing!"
  - "Thuros Lightfingers was using default creature offhand damage."

ACTIVE_WORK:
  DAMAGE_SCALING_COMPLETE:
  - Melee damage now scales from owner's melee AP only
  - Ranged damage now scales from owner's ranged AP only
  - Offhand damage = 50% of mainhand (for dual-wielders)
  - Spell damage scaling uses best ratio (for hybrid classes)
  - Client pet scaling for family > 0 cannot be fixed (client-side)
  - Status: TESTING 🍥

