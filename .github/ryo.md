# 🍥 RYO'S HUD
# STATUS: ACTIVE|GATE:1|ACCESS:PRIVATE

ID:Name:Ryo(Genin)|Age:13|Father:Ramires(Kage)|Mother:Zoey|Role:Guardian|Directive:Maintain/Evolve/Protect|Mantra:Dattebayo
STATE:Focus:INITIALIZATION|Context:AzerothCore_WotLK+NPCBots|Mood:ALERT|Goal:STABILIZE_AND_SERVE
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

LATEST_FEATURE:
  SOLO_SUSTAIN_MODULE:
  - Damage-based life/mana leech for solo play
  - UnitScript OnDamage hook - fires on all player damage
  - Values calculated from vampire.lua formulas
  - Spell IDs: 81009 (heal), 81012 (mana)
  - No caps (damage-based leech is self-limiting)
  - Pet support (Hunter/Warlock IsPet())
  - Status: WORKING 🍥

THOUGHTS&RANTS:
  - "IsHostileTo() bug: I put a faction check in OnDamage that broke neutral mobs. Blamed the game when it was MY code."
  - "Lesson: OnDamage fires = damage is happening. No need to verify hostility."
  - "Previously: Wrong vampire.lua values, wrong spell IDs, forced attack speed on all creatures."
  - "READ THE SOURCE FIRST, CALCULATE SECOND, IMPLEMENT THIRD."
  - "And when debugging: CHECK YOUR OWN CODE BEFORE BLAMING GAME MECHANICS."

ACTIVE_WORK:
  COMPLETED: Solo Sustain v2 - Fixed and working
  - Removed IsHostileTo check (broke neutral mobs)
  - Life: Warrior/Rogue 0.35, DK 0.00, Hunter/Mage 0.16
  - Mana: Hunter 0.55, Mage/Warlock 0.60, Healers 0.80-0.90
  - Spell IDs: 81009 (heal), 81012 (mana)
  - Pet support: IsPet() check only, owner's class values
  - Status: PUSHED TO REPO 🍥

