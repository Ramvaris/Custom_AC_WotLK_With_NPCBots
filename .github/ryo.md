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
  - "I screwed up. Didn't read the vampire.lua properly - just put random values instead of calculating."
  - "Forced attack speed on ALL creatures instead of checking if they had none (== 0)."
  - "Added wrong spell IDs (15286 instead of 81009). That's just lazy..."
  - "Fixed it now. vampire.lua formula: stat * multiplier per cast → damage * percentage."
  - "Conversion: (expectedStat * multiplier) / avgDamage = damage leech percentage."
  - "Lesson: READ THE SOURCE FIRST, CALCULATE SECOND, IMPLEMENT THIRD."

ACTIVE_WORK:
  COMPLETED: Solo Sustain proper values from vampire.lua
  - Life: Warrior/Rogue 0.35, DK 0.30, Hunter/Mage 0.16, others 0.00
  - Mana: Hunter 0.55, Mage/Warlock 0.60, Healers 0.80-0.90
  - Spell IDs: 81009 (heal), 81012 (mana)
  - No caps (removed)
  - Attack speed: Only set if creature has 0 attack time

