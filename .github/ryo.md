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
  [x] - Soul Keeper Guardian AI v3 🍥
  [x] - Soul Keeper Comprehensive Scaling System 🍥

LATEST_FEATURE:
  GUARDIAN_SCALING_SYSTEM_V2:
  - Comprehensive scaling: Melee, Spells, DoTs, Heals, HoTs, Shields, Thorns
  - Level-bracketed base DPS/HPS from 1-80 (1.5 → 750 at 80)
  - Gear detection: Compares melee AP, ranged AP, spell power - picks best
  - Mana scaling: 3-8% of max mana per spell (scales by owner level)
  - Immunity buffs: 60s minimum cooldown, only trigger when ACTUALLY taking damage
  - EXPLOIT FIX: Cooldown persistence across dismiss/summon cycles!
  - Saved per (ownerGUID, creatureEntry) → no swapping to bypass
  - Death does NOT save cooldowns (death = punishment)
  - Status: PUSHED TO GIT 🍥

THOUGHTS&RANTS:
  - "OnUnitUpdate runs in Unit::Update BEFORE Creature::UpdateAI. No conflict - separate hooks."
  - "If we cast, native AI sees UNIT_STATE_CASTING and skips. If native casts, we skip. Perfect sync."
  - "CombatAI only has AICOND_AGGRO/COMBAT/DIE. No AICOND_IDLE. Wild creatures DON'T buff out of combat!"
  - "Our AI injection ADDS out-of-combat behavior that even wild creatures don't have. Guardians superior."
  - "ScriptID check prevents overriding boss scripts. SmartAI creatures keep their smart behavior."
  - "Self-dispel added - guardian can remove polymorph/curse from itself now."
  - "Immunity bug found: TryCastBuff(creature, owner) checked caster damage, cast on owner. SELF-ONLY now!"

ACTIVE_WORK:
  BUGFIX: Immunity buffs cast on wrong target
  - Root cause: TryCastBuff checked if CASTER took damage but target was OWNER
  - Guardian took damage → immunity check passed → cast on owner instead of self!
  - Fix: Immunity buffs are now SELF-ONLY (target must equal caster)
  - Also fixed: GetSpellCooldownDelay → GetSpellCooldown (correct API)
  - Status: BUILT SUCCESSFULLY, READY FOR TEST 🍥
  - README updated with scaling formula table
  - Status: PUSHED TO GIT, TESTING NEEDED 🍥

