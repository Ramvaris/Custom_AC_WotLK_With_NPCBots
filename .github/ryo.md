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
  GOSSIP_MENU_COLLISION_FIX:
  - Soul Keeper gossip (.soul summon) was colliding with Lua gossip (.special)
  - Root cause: ClearMenus() does NOT reset _menuId field
  - When .special (menu_id 99999) was used first, Soul Keeper inherited that menu_id
  - Eluna handler for 99999 fired when clicking Soul Keeper options = NPC spawn!
  - FIX: Explicitly call SetMenuId(SOUL_KEEPER_GOSSIP_MENU_ID) after ClearGossipMenuFor()
  - FIX: Change OnPlayerGossipSelect to strict menu_id check (no OR with sender)
  - Status: BUILD SUCCESSFUL 🍥

THOUGHTS&RANTS:
  - "OnUnitUpdate runs in Unit::Update BEFORE Creature::UpdateAI. No conflict - separate hooks."
  - "If we cast, native AI sees UNIT_STATE_CASTING and skips. If native casts, we skip. Perfect sync."
  - "CombatAI only has AICOND_AGGRO/COMBAT/DIE. No AICOND_IDLE. Wild creatures DON'T buff out of combat!"
  - "Our AI injection ADDS out-of-combat behavior that even wild creatures don't have. Guardians superior."
  - "ScriptID check prevents overriding boss scripts. SmartAI creatures keep their smart behavior."
  - "Self-dispel added - guardian can remove polymorph/curse from itself now."

ACTIVE_WORK:
  COMPLETED: Soul Keeper Guardian AI Fix (v3)
  - Evade behavior: Soul Keeper guardians (UNIT_CREATED_BY_SPELL=81100) keep ALL auras
  - AI selection: Force CombatAI if creature has spells but no AIName AND no ScriptID
  - AI injection: OnUnitUpdate hook with per-guardian timers (support logic)
  - Native AI compatibility: OnUnitUpdate runs BEFORE UpdateAI, UNIT_STATE checks prevent conflicts
  - Heals: Emergency (35%) and normal (60%) HP thresholds, plus OOC healing to 95%
  - Dispels: Owner AND self (guardian can dispel itself now!)
  - Buffs: COMBAT buffs AND out-of-combat buffs (like wild counterparts)
  - State checks: CASTING, STUNNED, CONFUSED, FLEEING all respected
  - All spell casts: Mana cost, cooldown, and range validated
  - Status: BUILD SUCCESS, TESTING NEEDED 🍥

