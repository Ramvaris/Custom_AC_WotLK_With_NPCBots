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
  COMPILATION_CHECK:
  - Running build to identify broken modules
  - Expecting legacy code issues in /modules

THOUGHTS&RANTS:
  - "DK pet scaling auras check for NPC_RISEN_GHOUL entry... learned the hard way."
  - "Friendly NPC absorption = instant Varian exploit. Ramires caught that one!"
  - "Full manual scaling is more work but 100% reliable across all creatures."
  - "SPELL_PET_AVOIDANCE is the only truly generic pet aura. The rest are entry-locked."
  - "ObjectGUIDs are MONOTONICALLY INCREASING (never recycled during runtime)!"
  - "Was paranoid about GUID recycling - Ramires made me actually check the code."
  - "PetAI's OwnerAttackedBy() is great but brings the pet bar. UnitScript hooks are the answer!"
  - "REACT_DEFENSIVE + UnitScript hooks = owner-assist without pulling extra mobs."
  - "SetLevel(ownerLevel) is CRITICAL for guardians - without it CC spells get resisted!"
  - "MagicSpellHitResult uses getLevelForTarget() - level 10 murloc vs level 80 mob = -382% hit!"
  - "Guardian spell damage should be STAT-BASED from owner, not multiplied from creature template."
  - "Use SetStatFlatModifier(UNIT_MOD_RESISTANCE_START+school) for proper resistance handling."
  - "Guardian::UpdateMaxHealth() reads from UNIT_MOD_HEALTH, NOT SetMaxHealth()! Same for mana!"
  - "Guardian::UpdateDamagePhysical() adds AP/14*att_speed - zero AP to avoid double damage!"
  - "SetControlledByPlayer(true) + UNIT_FLAG_PLAYER_CONTROLLED = mobs add guardian to threat list!"
  - "Unit::SetMinion() sets these flags - manual summoning skips it, so SET THEM YOURSELF!"
  - "DPS formula (stat/14)*0.35 = PATHETIC damage. Stat*0.50 is proper WotLK endgame scaling!"
  - "WotLK endgame: BM Hunter pet ~40-50%, MM/SV pet ~20-25%. Target ~35% for Soul Keeper."
  - "Test with REAL gear, not placeholder! 707 SP + talents = 1250 DPS, not theoretical."

PLANNED_JUTSU:
  SOUL_KEEPER_MODULE:
  - Concept: Classless Guardian System (Creature Collection)
  - Commands: .soul absorb | .soul summon [#] | .soul dismiss | .soul rename <name> | .soul search <text>
  - Logic: DEAD ONLY capture, full manual scaling
  - Status: SHIPPED TO REPO 🍥🎉

ACTIVE_WORK:
  SOUL_KEEPER_MODULE:
  - [x] Commands: absorb, summon [index], dismiss, return, rename, search
  - [x] DEAD ONLY capture (fixed ally exploit)
  - [x] Full manual stat scaling (DK auras are entry-locked!)
  - [x] All guardians get auto-attack
  - [x] Resistances + Armor + Health/Mana + Base Stats (SetStatFlatModifier)
  - [x] Rename updates DB and memory
  - [x] Guardian death cleanup (no cooldown, just tracking)
  - [x] REACT_DEFENSIVE (won't pull extra mobs)
  - [x] Elite/boss flags removed on summon
  - [x] Owner-assist via UnitScript hooks (OnUnitEnterCombat + OnDamage)
  - [x] Guardian uses native AI for spells/abilities
  - [x] RemoveUnitTypeMask() added to Unit.h (no pet bar interference)
  - [x] SetLevel(ownerLevel) - CC spells no longer resisted!
  - [x] Spell damage = DPS formula * cast time (owner-based!)
  - [x] Combat-only restriction (no cooldown timer)
  - [x] Auto-swap on summon (dismiss old → summon new)
  - [x] Pet loot eligibility fix in core (IsCreatedByPlayer check)
  - [x] SetControlledByPlayer(true) + UNIT_FLAG_PLAYER_CONTROLLED for threat!
  - [x] UNIT_MOD_HEALTH/MANA for Guardian::UpdateMaxHealth compat
  - [x] Zero AP + weapon damage only for Guardian::UpdateDamagePhysical compat
  - [x] Pagination (12 per page, infinite scaling)
  - [x] .soul search for finding souls in massive collections
  - [x] 20% DPS at endgame, 40% while leveling (intentional design)
  - STATUS: SHIPPED 🍥🎉

