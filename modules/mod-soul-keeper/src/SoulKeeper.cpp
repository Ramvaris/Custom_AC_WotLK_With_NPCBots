/*
 * Soul Keeper Module - Implementation
 * Author: Ramvaris
 * License: Free to use, modify, distribute as long as you mention the original author
 * 
 * Capture creature souls and summon them as guardians
 * Pure C++ - No Lua dependency
 * 
 * COMMANDS:
 *   .soul absorb         - Capture target (DEAD creatures only)
 *   .soul dismiss        - Dismiss active guardian (combat-only restriction)
 *   .soul return         - Same as dismiss (for when guardian chases too far)
 *   .soul summon         - No arg: Open soul menu | With arg: Summon by index
 *   .soul rename <name>  - Rename currently summoned guardian
 *   .soul search <text>  - Search souls by partial name (case-insensitive)
 * 
 * FEATURES:
 *   - DEAD creatures only (prevents capturing Varian etc.)
 *   - One soul per creature entry (no duplicates)
 *   - REAL guardian summoning via SummonPropertiesEntry (proper Guardian class)
 *   - Player-based stat scaling: 80% HP, 70% mana, 35% armor, 40% resists
 *   - Does NOT interfere with Hunter/Warlock pet system (no pet bar)
 *   - Stats auto-refresh on combat end (equipment changes apply without resummon!)
 * 
 * STAT SELECTION (for hybrid classes):
 *   - Compares melee AP, ranged AP, and spell power using PROPER expected values
 *   - Level 80 expected: 4500 melee AP, 4000 ranged AP, 2800 spell power
 *   - Picks whichever stat produces the HIGHEST effective damage (normalized)
 *   - Example: Ret Paladin 2000 AP / 800 SP → melee wins (0.44 vs 0.29 ratio)
 *   - Example: Holy Paladin 500 AP / 2000 SP → spell wins (0.11 vs 0.71 ratio)
 * 
 * OWNER-ASSIST BEHAVIOR (via OnDamage hook):
 *   - Owner ATTACKS something → Guardian assists (works for ranged/melee)
 *   - Owner TAKES damage → Guardian defends (attacks the attacker)
 *   - Guardian uses its native AI for spells/abilities during combat
 *   - REACT_DEFENSIVE prevents guardian from pulling extra mobs
 * 
 * EVADE BEHAVIOR:
 *   - On combat end, guardian stats refresh from current owner gear
 *   - HP/Mana PERCENTAGE is preserved (not full heal on evade)
 *   - Player-cast buffs (Blessings, etc.) are preserved (core fix)
 *   - Buff contributions to HP/Mana are included in percentage calculation
 * 
 * HEAL LIMITATION:
 *   - Guardians use their native creature AI (spells, heals, buffs)
 *   - However, most creature AI only heals SELF or random friendly targets
 *   - Guardian will NOT specifically prioritize healing the OWNER
 *   - This would require custom AI per creature type (not implemented)
 */

#include "SoulKeeper.h"
#include "Chat.h"
#include "CombatAI.h"
#include "Creature.h"
#include "GameTime.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "ScriptDefines/UnitScript.h"
#include "SpellAuraEffects.h"
#include "Pet.h"
#include "PetDefines.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Language.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "SpellDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "DBCStores.h"
#include "TemporarySummon.h"
#include "Map.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <algorithm>

// =============================================================================
// Singleton Implementation
// =============================================================================
SoulKeeper* SoulKeeper::instance()
{
    static SoulKeeper instance;
    return &instance;
}

// =============================================================================
// Database Operations
// =============================================================================
void SoulKeeper::LoadSouls(Player* player)
{
    if (!player) return;

    uint32 guid = player->GetGUID().GetCounter();
    _caughtSouls[guid].clear();

    // ORDER BY caught_at ensures consistent soul ordering across server restarts
    QueryResult result = CharacterDatabase.Query(
        "SELECT creature_entry, custom_name, display_id, scale_factor FROM character_soul_keeper WHERE owner_guid = {} ORDER BY caught_at ASC",
        guid);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        SoulData soul;
        soul.creatureEntry = fields[0].Get<uint32>();
        soul.customName    = fields[1].Get<std::string>();
        soul.displayId     = fields[2].Get<uint32>();
        soul.scaleFactor   = fields[3].Get<float>();
        _caughtSouls[guid].push_back(soul);
    } while (result->NextRow());
}

void SoulKeeper::SaveSoul(Player* player, SoulData const& soul)
{
    if (!player) return;

    CharacterDatabase.Execute(
        "INSERT INTO character_soul_keeper (owner_guid, creature_entry, custom_name, display_id, scale_factor, caught_at) "
        "VALUES ({}, {}, '{}', {}, {}, {})",
        player->GetGUID().GetCounter(),
        soul.creatureEntry,
        soul.customName,
        soul.displayId,
        soul.scaleFactor,
        (uint64)time(nullptr)
    );
}

bool SoulKeeper::HasSoul(Player* player, uint32 entry)
{
    if (!player) return false;
    auto& souls = _caughtSouls[player->GetGUID().GetCounter()];
    for (const auto& soul : souls)
    {
        if (soul.creatureEntry == entry) return true;
    }
    return false;
}

// =============================================================================
// Utility Functions
// =============================================================================
bool SoulKeeper::IsInCombat(Player* player)
{
    if (!player) return true;
    return player->IsInCombat();
}

bool SoulKeeper::HasActiveGuardian(Player* player)
{
    if (!player) return false;
    auto it = _activeGuardians.find(player->GetGUID().GetCounter());
    if (it == _activeGuardians.end()) return false;
    
    // Verify guardian still exists and is alive
    // Note: GUIDs are monotonically increasing (never recycled during runtime)
    // so if GetCreature returns non-null, it's definitely our creature
    if (Creature* guardian = ObjectAccessor::GetCreature(*player, it->second))
    {
        if (guardian->IsAlive())
            return true;
    }
    
    // Clean up stale entry
    _activeGuardians.erase(player->GetGUID().GetCounter());
    return false;
}

std::string SoulKeeper::GetCreatureIconString(uint32 /*displayId*/)
{
    // Generic pet icon since DBC icons aren't directly accessible per displayId
    return "|TInterface\\Icons\\Ability_Hunter_BeastCall:20:20:-2:0|t";
}

// =============================================================================
// Cooldown Persistence: Save/Restore across dismiss/summon cycles
// Prevents exploit: .soul dismiss + .soul summon = FREE COOLDOWN RESET
// Cooldowns are stored per (ownerGUID, creatureEntry) so swapping guardians
// doesn't let you bypass cooldowns either (each guardian type tracks separately).
// =============================================================================
void SoulKeeper::SaveGuardianCooldowns(Creature* guardian, Player* owner)
{
    if (!guardian || !owner) return;
    
    uint32 ownerLow = owner->GetGUID().GetCounter();
    uint32 creatureEntry = guardian->GetEntry();
    uint32 now = getMSTime();
    
    // Get creature template to iterate over known spells
    CreatureTemplate const* cInfo = guardian->GetCreatureTemplate();
    if (!cInfo) return;
    
    // Clear old cooldowns for this guardian type
    _persistentCooldowns[ownerLow][creatureEntry].clear();
    
    // Save remaining cooldowns for each spell
    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
    {
        uint32 spellId = cInfo->spells[i];
        if (!spellId) continue;
        
        if (guardian->HasSpellCooldown(spellId))
        {
            // Get remaining cooldown time
            uint32 remaining = guardian->GetSpellCooldown(spellId);
            if (remaining > 0)
            {
                // Store absolute end time
                _persistentCooldowns[ownerLow][creatureEntry][spellId] = now + remaining;
            }
        }
    }
}

void SoulKeeper::RestoreGuardianCooldowns(Creature* guardian, Player* owner)
{
    if (!guardian || !owner) return;
    
    uint32 ownerLow = owner->GetGUID().GetCounter();
    uint32 creatureEntry = guardian->GetEntry();
    uint32 now = getMSTime();
    
    // Check if we have persistent cooldowns for this guardian type
    auto ownerIt = _persistentCooldowns.find(ownerLow);
    if (ownerIt == _persistentCooldowns.end()) return;
    
    auto entryIt = ownerIt->second.find(creatureEntry);
    if (entryIt == ownerIt->second.end()) return;
    
    // Restore each cooldown that hasn't expired yet
    for (auto& [spellId, endTime] : entryIt->second)
    {
        if (endTime > now)
        {
            uint32 remaining = endTime - now;
            guardian->AddSpellCooldown(spellId, 0, remaining);
        }
    }
    
    // Clear restored cooldowns (they're now on the guardian)
    entryIt->second.clear();
}

// =============================================================================
// Gossip Menu: Soul List (Paginated)
// Shows captured souls with [index] for .soul summon command
// Supports pagination for collectors with 400+ souls
// =============================================================================
void SoulKeeper::ShowSoulList(Player* player, uint32 page)
{
    if (!player) return;

    uint32 playerGuid = player->GetGUID().GetCounter();

    // Ensure souls are loaded
    auto& souls = _caughtSouls[playerGuid];
    if (souls.empty())
    {
        LoadSouls(player);
    }

    // Store current page for next/prev navigation
    _currentGossipPage[playerGuid] = page;

    ClearGossipMenuFor(player);
    
    // CRITICAL: Explicitly set our menu_id to prevent collision with Lua gossip menus
    // ClearMenus() does NOT reset menu_id, so if .special (menu_id 99999) was used before,
    // Eluna would fire its handler when player clicks our options!
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(SOUL_KEEPER_GOSSIP_MENU_ID);

    if (souls.empty())
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
            "|cff888888No souls captured yet. Target a dead enemy and use .soul absorb|r", 
            SOUL_KEEPER_GOSSIP_SENDER, SOUL_ACTION_CLOSE);
    }
    else
    {
        // === PAGINATION MATH ===
        uint32 totalSouls = (uint32)souls.size();
        uint32 totalPages = (totalSouls + SOULS_PER_PAGE - 1) / SOULS_PER_PAGE;
        uint32 startIndex = page * SOULS_PER_PAGE;
        uint32 endIndex = std::min(startIndex + SOULS_PER_PAGE, totalSouls);

        // === NAVIGATION AT TOP (Next first for fast forward clicking) ===
        if (page < totalPages - 1)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
                "|TInterface\\Icons\\Ability_Druid_Dash:20:20:-2:0|t Next Page >>", 
                SOUL_KEEPER_GOSSIP_SENDER, SOUL_ACTION_NEXT_PAGE);
        }
        if (page > 0)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
                "|TInterface\\Icons\\Ability_Druid_Dash_Orange:20:20:-2:0|t << Previous Page", 
                SOUL_KEEPER_GOSSIP_SENDER, SOUL_ACTION_PREV_PAGE);
        }

        // Page info as text (does nothing on click, purely informational)
        if (totalPages > 1)
        {
            std::string pageInfo = "|cff666666[ Page " + std::to_string(page + 1) + " / " + std::to_string(totalPages) + 
                                   " - " + std::to_string(totalSouls) + " souls total ]|r";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, pageInfo, SOUL_KEEPER_GOSSIP_SENDER, SOUL_ACTION_CLOSE);
        }

        // Soul list for current page
        for (uint32 i = startIndex; i < endIndex; ++i)
        {
            const SoulData& soul = souls[i];
            std::string icon = GetCreatureIconString(soul.displayId);
            // Display as 1-indexed for user, store actual array index in action
            std::string label = "|cff00ff00[" + std::to_string(i + 1) + "]|r " + icon + " " + soul.customName;
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, label, SOUL_KEEPER_GOSSIP_SENDER, SOUL_ACTION_SUMMON_BASE + i);
        }
    }

    // Close button at the very end
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
        "|TInterface\\Icons\\Misc_ArrowLeft:20:20:-2:0|t Close", 
        SOUL_KEEPER_GOSSIP_SENDER, SOUL_ACTION_CLOSE);

    SendGossipMenuFor(player, SOUL_KEEPER_NPC_TEXT_ID, player->GetGUID());
}

bool SoulKeeper::HandleGossipSelect(Player* player, uint32 /*sender*/, uint32 action)
{
    if (!player) return false;

    uint32 playerGuid = player->GetGUID().GetCounter();
    CloseGossipMenuFor(player);

    switch (action)
    {
        case SOUL_ACTION_CLOSE:
            return true;

        case SOUL_ACTION_PREV_PAGE:
        {
            uint32 currentPage = _currentGossipPage[playerGuid];
            if (currentPage > 0)
                ShowSoulList(player, currentPage - 1);
            else
                ShowSoulList(player, 0);
            return true;
        }

        case SOUL_ACTION_NEXT_PAGE:
        {
            uint32 currentPage = _currentGossipPage[playerGuid];
            ShowSoulList(player, currentPage + 1);
            return true;
        }

        default:
            // Summon action (SOUL_ACTION_SUMMON_BASE + index)
            if (action >= SOUL_ACTION_SUMMON_BASE)
            {
                uint32 index = action - SOUL_ACTION_SUMMON_BASE;
                auto& souls = _caughtSouls[playerGuid];
                if (index < souls.size())
                {
                    SummonGuardian(player, souls[index].creatureEntry);
                }
            }
            return true;
    }

    return false;
}

// =============================================================================
// Core Logic: Capture Soul
// DEAD ONLY - prevents exploits (capturing faction leaders, quest NPCs, etc.)
// One soul per creature entry (no duplicates)
// Corpse NOT deleted (quest safety)
// =============================================================================
void SoulKeeper::AddGuardian(Player* player, Unit* victim)
{
    if (!player || !victim || !victim->ToCreature())
        return;

    Creature* creature = victim->ToCreature();

    // DEAD ONLY - no alive capture (prevents capturing Varian, faction leaders, etc.)
    if (!creature->isDead())
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Target must be dead! Kill it first.|r");
        return;
    }

    // Check Duplicates
    if (HasSoul(player, creature->GetEntry()))
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff6600You already have this soul!|r");
        return;
    }

    // Create Soul Data
    SoulData newSoul;
    newSoul.creatureEntry = creature->GetEntry();
    newSoul.customName    = creature->GetName();
    newSoul.displayId     = creature->GetDisplayId();
    // Use GetNativeObjectScale() to get the DATABASE-DEFINED size, not the current scale.
    // GetObjectScale() returns whatever the creature's scale is RIGHT NOW (e.g., shrunk by
    // hunter pet level-scaling or other effects). GetNativeObjectScale() returns the scale
    // from creature_template_model - the creature's INTENDED visual size.
    newSoul.scaleFactor   = creature->GetNativeObjectScale();

    // Add to Memory
    _caughtSouls[player->GetGUID().GetCounter()].push_back(newSoul);

    // Persist to DB
    SaveSoul(player, newSoul);

    // Simple green flash (Soul Shard visual) - no spell cast, just message
    ChatHandler(player->GetSession()).PSendSysMessage("|cff00ff00[Soul Keeper]|r Soul of |cffffcc00{}|r captured!", newSoul.customName);
}

// =============================================================================
// Core Logic: Summon Guardian
// - Uses REAL guardian summoning via SummonPropertiesEntry
// - InitStatsForLevel() for proper level-based health/stats
// - REACT_AGGRESSIVE for automatic combat behavior
// =============================================================================
void SoulKeeper::SummonGuardian(Player* player, uint32 entry)
{
    if (!player) return;

    // Cannot summon while in combat
    if (IsInCombat(player))
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Cannot summon guardians while in combat!|r");
        return;
    }

    // If already have a guardian, auto-dismiss it first (swap functionality)
    if (HasActiveGuardian(player))
    {
        uint32 guid = player->GetGUID().GetCounter();
        auto it = _activeGuardians.find(guid);
        if (it != _activeGuardians.end())
        {
            if (Creature* oldGuardian = ObjectAccessor::GetCreature(*player, it->second))
            {
                _guardianScaling.erase(oldGuardian->GetGUID());
                _guardianAITimer.erase(oldGuardian->GetGUID());
                _guardianLastDamage.erase(oldGuardian->GetGUID());
                oldGuardian->DespawnOrUnsummon();
            }
            _activeGuardians.erase(it);
        }
    }

    // Find the Soul
    auto& souls = _caughtSouls[player->GetGUID().GetCounter()];
    SoulData* targetSoul = nullptr;
    for (auto& soul : souls)
    {
        if (soul.creatureEntry == entry)
        {
            targetSoul = &soul;
            break;
        }
    }

    if (!targetSoul)
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000You don't possess this soul.|r");
        return;
    }

    // === PROPER GUARDIAN SUMMONING ===
    // Use SummonPropertiesEntry ID 61 (guardian type) to create a proper Guardian object
    // This makes Map::SummonCreature create a Guardian class with proper initialization
    SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(61);
    if (!properties)
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Internal error: Guardian properties not found.|r");
        return;
    }

    Position pos = player->GetPosition();
    TempSummon* summon = player->GetMap()->SummonCreature(entry, pos, properties, 0, player);
    
    if (!summon)
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Failed to summon guardian.|r");
        return;
    }

    // === The summon is now a proper Guardian object ===
    Creature* guardian = summon->ToCreature();

    // === OWNERSHIP ===
    guardian->SetOwnerGUID(player->GetGUID());
    guardian->SetCreatorGUID(player->GetGUID());
    guardian->SetFaction(player->GetFaction());
    guardian->SetLootMode(0); // No loot

    // === PLAYER CONTROLLED FLAG ===
    // CRITICAL: These flags are normally set by Unit::SetMinion() but we don't call that.
    // Without these, mobs won't add the guardian to their threat list properly!
    guardian->SetControlledByPlayer(true);
    guardian->SetUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED);

    // === LOOT ELIGIBILITY FIX ===
    // CRITICAL: Ensure m_CreatedByPlayer is TRUE so guardian damage counts as player damage.
    // Without this, LowerPlayerDamageReq won't mark the mob as "damaged by player" and
    // IsDamageEnoughForLootingAndReward() returns false = no loot for player!
    // This is normally set in TempSummon::InitStats but we explicitly ensure it here.
    guardian->m_CreatedByPlayer = true;

    // === GUARDIAN FLAGS - NOT a controllable pet! ===
    // Remove NPC interaction flags - this is a combat guardian
    guardian->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
    
    // Explicitly remove CONTROLLABLE_GUARDIAN flag to avoid pet bar/Hunter/Warlock interference
    // This makes it a regular guardian that uses creature AI, not PetAI
    if (summon->HasUnitTypeMask(UNIT_MASK_CONTROLLABLE_GUARDIAN))
    {
        summon->RemoveUnitTypeMask(UNIT_MASK_CONTROLLABLE_GUARDIAN);
    }
    
    // === CRITICAL: Ensure UNIT_MASK_GUARDIAN is set ===
    // SummonPropertiesEntry 61 should create a Guardian class, but the type mask
    // might not be set correctly for all creature types. Critters especially
    // need this flag or they'll be treated as passive non-combatants.
    // This bypasses the critter checks in UpdateMoveInLineOfSightState().
    if (!summon->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
    {
        summon->AddUnitTypeMask(UNIT_MASK_GUARDIAN);
    }
    
    // === INIT STATS FOR LEVEL (Proper health/damage scaling) ===
    // If it's a Guardian class, use InitStatsForLevel for proper stats
    if (summon->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
    {
        ((Guardian*)summon)->InitStatsForLevel(player->GetLevel());
    }
    else
    {
        // Fallback for non-guardian summons
        guardian->SetLevel(player->GetLevel());
    }

    // === REACT STATE + OWNER-ASSIST ===
    // REACT_DEFENSIVE: Guardian doesn't auto-pull mobs
    // UnitScript hooks handle owner-assist:
    //   - OnUnitEnterCombat: Guardian attacks when owner attacks
    //   - OnDamage: Guardian defends when owner is hit
    // The guardian's native AI handles spells/abilities in combat
    guardian->SetReactState(REACT_DEFENSIVE);
    
    // === CRITICAL: Recalculate line-of-sight movement state ===
    // Critters/passive creatures have m_moveInLineOfSightDisabled=true by default.
    // Now that we've added UNIT_MASK_GUARDIAN, recalculating will enable LOS.
    // This allows guardians to properly aggro and be targeted by mobs.
    guardian->UpdateMoveInLineOfSightState();

    // === Enable auto-attack ONLY if creature has no attack speed set ===
    // Don't override natural attack speeds - only set fallback for casters/critters
    if (guardian->GetAttackTime(BASE_ATTACK) == 0)
        guardian->SetAttackTime(BASE_ATTACK, 2000);
    if (guardian->GetAttackTime(OFF_ATTACK) == 0)
        guardian->SetAttackTime(OFF_ATTACK, 2000);
    if (guardian->GetAttackTime(RANGED_ATTACK) == 0)
        guardian->SetAttackTime(RANGED_ATTACK, 2000);

    // === Apply our custom scaling on TOP of InitStatsForLevel ===
    ScaleGuardian(guardian, player);

    // === RESTORE VISUAL SCALE FROM DATABASE (Native Scale) ===
    // Guardians should look EXACTLY like their database counterpart.
    // The scaleFactor is captured from GetNativeObjectScale() - the creature_template_model
    // defined size, NOT current scale (which could be shrunk by pet level-scaling).
    guardian->SetObjectScale(targetSoul->scaleFactor);
    
    // === RESTORE COOLDOWNS from previous summon (prevent dismiss/summon exploit!) ===
    // If player dismissed this guardian type earlier, restored cooldowns still apply.
    RestoreGuardianCooldowns(guardian, player);

    // === Follow Master (RIGHT side to avoid overlapping Hunter/Warlock pets on LEFT) ===
    // CRITICAL: Must also set m_followAngle via SetFollowAngle(), otherwise when AI or
    // other code calls MoveFollow(owner, dist, GetFollowAngle()), it uses the default LEFT!
    // Guardian inherits from Minion, so we can cast directly.
    if (summon->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
    {
        ((Minion*)summon)->SetFollowAngle(-PET_FOLLOW_ANGLE);
    }
    guardian->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, -PET_FOLLOW_ANGLE);

    // === FORCE CombatAI IF CREATURE HAS SPELLS BUT NO AI ===
    // Default AI for hostile creatures is AggressorAI (melee only, no spells).
    // CombatAI handles spell casting via events. If creature_template has spells
    // but no explicit AIName AND no ScriptName, the guardian would never use its spells.
    // 
    // IMPORTANT: Do NOT override if:
    // - AIName is set (SmartAI, CombatAI, etc. - they handle spells)
    // - ScriptID is set (scripted AI like boss scripts - they're smarter than CombatAI)
    // 
    // Only force CombatAI for "dumb" creatures with spells but no AI configuration.
    CreatureTemplate const* cInfo = guardian->GetCreatureTemplate();
    bool hasSpells = false;
    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
    {
        if (cInfo->spells[i] != 0)
        {
            hasSpells = true;
            break;
        }
    }
    if (hasSpells && cInfo->AIName.empty() && cInfo->ScriptID == 0)
    {
        // AIM_Initialize with CombatAI ensures creature uses its database spells
        guardian->AIM_Initialize(new CombatAI(guardian));
    }

    // Track Active Guardian
    _activeGuardians[player->GetGUID().GetCounter()] = guardian->GetGUID();

    // === CUSTOM NAME: Apply the player's custom name (from .soul rename) ===
    // Also set pet number and timestamp to make client query the pet name
    if (!targetSoul->customName.empty())
    {
        guardian->SetName(targetSoul->customName);
        // Pet number can be any unique value - use low GUID as identifier
        guardian->SetUInt32Value(UNIT_FIELD_PETNUMBER, guardian->GetGUID().GetCounter());
        // Timestamp triggers client to query CMSG_PET_NAME_QUERY
        guardian->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(GameTime::GetGameTime().count()));
    }

    ChatHandler(player->GetSession()).PSendSysMessage("|cff00ff00Summoned {}!|r", targetSoul->customName);
}

// =============================================================================
// Core Logic: Stat Scaling (Player-Based)
// Stats are SET to percentages of owner stats, not creature template.
// This ensures all captured creatures scale uniformly with the player.
//
// Stats scaled (from OWNER):
// - Health: 80% of owner
// - Mana: 70% of owner
// - Armor: 35% of owner
// - Resistances: 40% of owner (all schools)
// - Damage: max(MeleeAP, RangedAP, SpellPower) / 14 = bonusDPS
// - Avoidance: Standard 75% AoE damage reduction
// - Level: Set to owner level (critical for spell hit/resist calculations!)
// =============================================================================
void SoulKeeper::ScaleGuardian(Creature* guardian, Player* owner)
{
    if (!guardian || !owner) return;

    uint32 ownerLevel = owner->GetLevel();

    // === CRITICAL: Set guardian level to owner level ===
    // This is REQUIRED for spell hit/resist calculations!
    // A level 10 murloc casting on a level 80 mob = guaranteed miss otherwise
    // MagicSpellHitResult uses getLevelForTarget() for hit chance calculation
    guardian->SetLevel(ownerLevel);

    // === AVOIDANCE: Standard pet AoE damage reduction ===
    guardian->AddAura(SPELL_PET_AVOIDANCE, guardian);
    
    // === SOUL KEEPER MARKER: Identifies this as OUR guardian ===
    // Core's RemoveEvadeAuras checks UNIT_CREATED_BY_SPELL for this value
    // This lets guardians keep buffs AND debuffs after combat (fairness!)
    // No actual spell needs to exist - it's just a marker value
    guardian->SetUInt32Value(UNIT_CREATED_BY_SPELL, SPELL_SOUL_KEEPER_GUARDIAN);

    // === HEALTH: 80% of owner (minimum: level * 20) ===
    // CRITICAL: Must use SetStatFlatModifier because Guardian::UpdateMaxHealth()
    // recalculates HP from UNIT_MOD_HEALTH, NOT from SetMaxHealth()!
    uint32 ownerHealth = (uint32)(owner->GetMaxHealth() * 0.8f);
    uint32 finalHealth = std::max(ownerHealth, ownerLevel * 20u);
    guardian->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)finalHealth);
    guardian->SetCreateHealth(finalHealth);
    guardian->SetMaxHealth(finalHealth);
    guardian->SetHealth(finalHealth);

    // === MANA: 70% of owner (if mana-based) ===
    // Same issue: Guardian::UpdateMaxPower() uses UNIT_MOD_MANA, not SetMaxPower()
    if (guardian->GetPowerType() == POWER_MANA)
    {
        uint32 ownerMana = (uint32)(owner->GetMaxPower(POWER_MANA) * 0.7f);
        uint32 finalMana = std::max(ownerMana, ownerLevel * 15u);
        guardian->SetStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, (float)finalMana);
        guardian->SetCreateMana(finalMana);
        guardian->SetMaxPower(POWER_MANA, finalMana);
        guardian->SetPower(POWER_MANA, finalMana);
    }
    else
    {
        guardian->SetMaxPower(POWER_ENERGY, 100);
        guardian->SetPower(POWER_ENERGY, 100);
    }

    // === ARMOR: 35% of owner ===
    // Using SetStatFlatModifier to work with guardian stat system properly
    int32 ownerArmor = (int32)(owner->GetArmor() * 0.35f);
    guardian->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, (float)ownerArmor);
    guardian->SetArmor(ownerArmor);

    // === RESISTANCES: 40% of owner (all magic schools) ===
    // Using SetStatFlatModifier for proper stat handling
    for (uint8 school = SPELL_SCHOOL_HOLY; school < MAX_SPELL_SCHOOL; ++school)
    {
        int32 ownerResist = owner->GetResistance(SpellSchools(school));
        int32 guardianResist = (int32)(ownerResist * 0.4f);
        guardian->SetStatFlatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + school), BASE_VALUE, (float)guardianResist);
        guardian->SetResistance(SpellSchools(school), guardianResist);
    }

    // === DAMAGE SCALING ===
    // Get owner's stats for each damage type
    float meleeAP  = owner->GetTotalAttackPowerValue(BASE_ATTACK);
    float rangedAP = owner->GetTotalAttackPowerValue(RANGED_ATTACK);
    float maxSP    = 0.0f;
    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
    {
        float sp = (float)owner->SpellBaseDamageBonusDone((SpellSchoolMask)(1 << i));
        if (sp > maxSP) maxSP = sp;
    }

    // === LEVEL BRACKET DPS FORMULA ===
    // Based on typical WotLK 3.3.5a player DPS and stats per level bracket.
    // Target: 25% of player DPS at all levels (20-30% acceptable range).
    // Now with PROPER expected values per stat type!
    float targetDPS;
    float expectedMelee;   // Expected melee AP at this level
    float expectedRanged;  // Expected ranged AP at this level
    float expectedSpell;   // Expected spell power at this level
    
    // 10-level brackets with separate expected values per stat type
    // At level 80: melee ~4500 AP, ranged ~4000 RAP, caster ~2800 SP
    // Other brackets scale proportionally (melee/spell ratio ~1.6:1)
    if (ownerLevel <= 10)
    {
        targetDPS = 1.5f;
        expectedMelee = 50.0f;   expectedRanged = 45.0f;   expectedSpell = 30.0f;
    }
    else if (ownerLevel <= 20)
    {
        targetDPS = 5.0f;
        expectedMelee = 130.0f;  expectedRanged = 115.0f;  expectedSpell = 80.0f;
    }
    else if (ownerLevel <= 30)
    {
        targetDPS = 12.0f;
        expectedMelee = 240.0f;  expectedRanged = 215.0f;  expectedSpell = 150.0f;
    }
    else if (ownerLevel <= 40)
    {
        targetDPS = 25.0f;
        expectedMelee = 450.0f;  expectedRanged = 400.0f;  expectedSpell = 280.0f;
    }
    else if (ownerLevel <= 50)
    {
        targetDPS = 45.0f;
        expectedMelee = 720.0f;  expectedRanged = 640.0f;  expectedSpell = 450.0f;
    }
    else if (ownerLevel <= 60)
    {
        targetDPS = 85.0f;
        expectedMelee = 1120.0f; expectedRanged = 1000.0f; expectedSpell = 700.0f;
    }
    else if (ownerLevel <= 70)
    {
        targetDPS = 175.0f;
        expectedMelee = 1920.0f; expectedRanged = 1700.0f; expectedSpell = 1200.0f;
    }
    else
    {
        targetDPS = 750.0f;
        expectedMelee = 4500.0f; expectedRanged = 4000.0f; expectedSpell = 2800.0f;
    }

    // === EFFECTIVE CONTRIBUTION: Which stat produces the most DPS? ===
    // Compare normalized ratios (stat / expectedForType), not raw values!
    // A Paladin with 2000 AP and 800 SP: melee = 2000/4500 = 0.44, spell = 800/2800 = 0.29
    // → Melee wins. But a Holy Pally with 500 AP and 2000 SP: melee = 0.11, spell = 0.71
    // → Spell wins. This is the CORRECT behavior for hybrid classes!
    float meleeRatio  = meleeAP / expectedMelee;
    float rangedRatio = rangedAP / expectedRanged;
    float spellRatio  = maxSP / expectedSpell;

    // Pick the winner (whichever normalized ratio is highest)
    float gearRatio;
    if (meleeRatio >= rangedRatio && meleeRatio >= spellRatio)
        gearRatio = meleeRatio;
    else if (rangedRatio >= meleeRatio && rangedRatio >= spellRatio)
        gearRatio = rangedRatio;
    else
        gearRatio = spellRatio;

    // Floor at 50% (naked characters still get reasonable guardian)
    gearRatio = std::max(0.5f, gearRatio);

    // Scale guardian DPS by gear quality
    float totalDPS = targetDPS * gearRatio;

    // === MELEE DAMAGE ===
    // Use ACTUAL creature attack speed (not hardcoded 2.0s)
    // Damage per hit = DPS * attackTimeSeconds
    float meleeAttackTime = (float)guardian->GetAttackTime(BASE_ATTACK);
    if (meleeAttackTime <= 0.0f) meleeAttackTime = 2000.0f;  // Default if not set

    float meleeDamagePerHit = totalDPS * (meleeAttackTime / 1000.0f);
    
    // === RANGED DAMAGE ===
    // Same formula but using ranged attack speed
    float rangedAttackTime = (float)guardian->GetAttackTime(RANGED_ATTACK);
    if (rangedAttackTime <= 0.0f) rangedAttackTime = 2000.0f;  // Default if not set

    float rangedDamagePerHit = totalDPS * (rangedAttackTime / 1000.0f);
    
    // === DAMAGE SCALING (Guardian-compatible) ===
    // Guardian::UpdateDamagePhysical calculates: (BASE_VALUE + AP/14*att_speed + weapon_damage)
    // To avoid double-counting, we:
    // 1. Set AP to 0 (eliminate AP contribution)
    // 2. Set UNIT_MOD_DAMAGE_* BASE_VALUE to 0
    // 3. Set weapon damage to our target (this becomes the final damage)
    guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_DAMAGE_MAINHAND, BASE_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_DAMAGE_RANGED, BASE_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, 0.0f);
    
    // === MELEE WEAPON DAMAGE ===
    guardian->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, meleeDamagePerHit * 0.9f);
    guardian->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, meleeDamagePerHit * 1.1f);
    guardian->UpdateAttackPowerAndDamage(false);
    guardian->UpdateDamagePhysical(BASE_ATTACK);
    
    // === RANGED WEAPON DAMAGE ===
    guardian->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, rangedDamagePerHit * 0.9f);
    guardian->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, rangedDamagePerHit * 1.1f);
    guardian->UpdateAttackPowerAndDamage(true);  // true = ranged
    guardian->UpdateDamagePhysical(RANGED_ATTACK);
    
    // Verify final values are set (safety net for client display)
    guardian->SetStatFloatValue(UNIT_FIELD_MINDAMAGE, meleeDamagePerHit * 0.9f);
    guardian->SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, meleeDamagePerHit * 1.1f);
    guardian->SetStatFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, rangedDamagePerHit * 0.9f);
    guardian->SetStatFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, rangedDamagePerHit * 1.1f);
    guardian->SetMaxHealth(finalHealth);
    guardian->SetHealth(finalHealth);

    // === SPELL DAMAGE SCALING INFO ===
    // Store pre-calculated gear ratio for spell damage scaling in hooks.
    // Spell hooks use gearRatio directly (already normalized, no division needed).
    GuardianScalingInfo scalingInfo;
    scalingInfo.gearRatio = gearRatio;  // Pre-calculated, ready to use
    scalingInfo.creatureLevel = guardian->GetCreatureTemplate()->maxlevel;
    scalingInfo.ownerLevel = ownerLevel;
    _guardianScaling[guardian->GetGUID()] = scalingInfo;
}

// =============================================================================
// Core Logic: Dismiss Guardian
// Only allowed out of combat to prevent exploit swapping
// =============================================================================
void SoulKeeper::DismissGuardian(Player* player)
{
    if (!player) return;

    // Cannot dismiss while in combat
    if (IsInCombat(player))
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Cannot dismiss guardians while in combat!|r");
        return;
    }

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _activeGuardians.find(guid);
    if (it != _activeGuardians.end())
    {
        // GUIDs never recycle during runtime - if creature exists, it's ours
        if (Creature* guardian = ObjectAccessor::GetCreature(*player, it->second))
        {
            // SAVE COOLDOWNS before despawn (prevent dismiss/summon exploit!)
            SaveGuardianCooldowns(guardian, player);
            
            // Clean up scaling data
            _guardianScaling.erase(guardian->GetGUID());
            _guardianAITimer.erase(guardian->GetGUID());
            _guardianLastDamage.erase(guardian->GetGUID());
            guardian->DespawnOrUnsummon();
        }
        _activeGuardians.erase(it);

        ChatHandler(player->GetSession()).SendSysMessage("|cff888888Guardian dismissed.|r");
    }
    else
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cff888888No active guardian to dismiss.|r");
    }
}

// =============================================================================
// Core Logic: Return Guardian
// Same as dismiss - despawns guardian and cleans up tracking
// Used for .soul return when guardian is chasing enemies too far
// =============================================================================
void SoulKeeper::ReturnGuardian(Player* player)
{
    // Functionally identical to dismiss - just different message
    if (!player) return;

    // Cannot recall while in combat
    if (IsInCombat(player))
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Cannot recall guardians while in combat!|r");
        return;
    }

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _activeGuardians.find(guid);
    if (it != _activeGuardians.end())
    {
        if (Creature* guardian = ObjectAccessor::GetCreature(*player, it->second))
        {
            // SAVE COOLDOWNS before despawn (prevent dismiss/summon exploit!)
            SaveGuardianCooldowns(guardian, player);
            
            // Clean up scaling data
            _guardianScaling.erase(guardian->GetGUID());
            _guardianAITimer.erase(guardian->GetGUID());
            _guardianLastDamage.erase(guardian->GetGUID());
            guardian->DespawnOrUnsummon();
        }
        _activeGuardians.erase(it);

        ChatHandler(player->GetSession()).SendSysMessage("|cff888888Guardian recalled.|r");
    }
    else
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cff888888No active guardian to recall.|r");
    }
}

// =============================================================================
// Core Logic: Guardian Death Handler
// Called by UnitScript when a creature dies
// If it's our guardian, cleans up tracking (no cooldown - death is punishment enough)
// =============================================================================
void SoulKeeper::OnGuardianDeath(Creature* guardian)
{
    if (!guardian) return;

    ObjectGuid ownerGuid = guardian->GetOwnerGUID();
    if (!ownerGuid) return;

    uint32 playerGuidLow = ownerGuid.GetCounter();

    // Check if this creature is tracked as someone's guardian
    auto it = _activeGuardians.find(playerGuidLow);
    if (it != _activeGuardians.end() && it->second == guardian->GetGUID())
    {
        // Our guardian died - clean up
        _guardianScaling.erase(guardian->GetGUID());
        _guardianAITimer.erase(guardian->GetGUID());
        _guardianLastDamage.erase(guardian->GetGUID());
        _activeGuardians.erase(it);

        // Notify owner if online
        if (Player* owner = ObjectAccessor::FindPlayer(ownerGuid))
        {
            ChatHandler(owner->GetSession()).SendSysMessage("|cffff6600Your guardian has fallen!|r");
        }
    }
}

// =============================================================================
// Core Logic: Rename Guardian
// Renames the currently summoned guardian and updates the database
// =============================================================================
void SoulKeeper::RenameGuardian(Player* player, std::string const& newName)
{
    if (!player) return;

    uint32 guid = player->GetGUID().GetCounter();
    
    // Check if player has an active guardian
    auto it = _activeGuardians.find(guid);
    if (it == _activeGuardians.end())
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000You must have an active guardian to rename.|r");
        return;
    }

    Creature* guardian = ObjectAccessor::GetCreature(*player, it->second);
    if (!guardian)
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Guardian not found.|r");
        _activeGuardians.erase(it);
        return;
    }

    // GUIDs are monotonically increasing (never recycled during runtime)
    // If ObjectAccessor returns non-null for our stored GUID, it's definitely ours

    uint32 creatureEntry = guardian->GetEntry();

    // Find the soul in memory and update it
    auto& souls = _caughtSouls[guid];
    for (auto& soul : souls)
    {
        if (soul.creatureEntry == creatureEntry)
        {
            std::string oldName = soul.customName;
            soul.customName = newName;

            // Update database
            CharacterDatabase.Execute(
                "UPDATE character_soul_keeper SET custom_name = '{}' WHERE owner_guid = {} AND creature_entry = {}",
                newName, guid, creatureEntry);

            // Apply name to currently summoned guardian IMMEDIATELY (not just on resummon)
            guardian->SetName(newName);
            // Force client to re-query the pet name by updating the timestamp
            guardian->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(GameTime::GetGameTime().count()));

            ChatHandler(player->GetSession()).PSendSysMessage("|cff00ff00Renamed '{}' to '{}'!|r", oldName, newName);
            return;
        }
    }

    ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Soul not found in your collection.|r");
}

// =============================================================================
// PlayerScript: Gossip Menu Hook + Login
// =============================================================================
class SoulKeeper_PlayerScript : public PlayerScript
{
public:
    SoulKeeper_PlayerScript() : PlayerScript("SoulKeeper_PlayerScript") { }

    void OnPlayerGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override
    {
        // STRICT menu_id check - only handle OUR gossip menu (89999)
        // This prevents collision with Lua gossip (e.g., .special uses 99999)
        // The sender check is redundant now but kept for extra safety
        if (menu_id == SOUL_KEEPER_GOSSIP_MENU_ID && sender == SOUL_KEEPER_GOSSIP_SENDER)
        {
            // Handle ALL Soul Keeper actions (0-4 = menu actions, 100+ = summon by index)
            if (action <= SOUL_ACTION_NEXT_PAGE || action >= SOUL_ACTION_SUMMON_BASE)
            {
                sSoulKeeper->HandleGossipSelect(player, sender, action);
            }
        }
    }

    void OnPlayerLogin(Player* player) override
    {
        // Pre-load souls on login for faster menu access
        sSoulKeeper->LoadSouls(player);
    }
};

// =============================================================================
// CommandScript: .soul Commands
// SIMPLIFIED: absorb, dismiss, summon [index]
// =============================================================================
using namespace Acore::ChatCommands;

class SoulKeeper_CommandScript : public CommandScript
{
public:
    SoulKeeper_CommandScript() : CommandScript("SoulKeeper_CommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable soulCommandTable =
        {
            { "absorb",  HandleAbsorbCommand,  SEC_PLAYER, Console::No },
            { "summon",  HandleSummonCommand,  SEC_PLAYER, Console::No },
            { "dismiss", HandleDismissCommand, SEC_PLAYER, Console::No },
            { "return",  HandleReturnCommand,  SEC_PLAYER, Console::No },
            { "rename",  HandleRenameCommand,  SEC_PLAYER, Console::No },
            { "search",  HandleSearchCommand,  SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "soul", soulCommandTable }
        };

        return commandTable;
    }

    // .soul absorb - Capture target (dead enemy OR alive ally)
    static bool HandleAbsorbCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        Unit* target = player->GetSelectedUnit();
        if (!target)
        {
            handler->SendSysMessage("|cffff0000You must target a creature.|r");
            return true;
        }

        Creature* creature = target->ToCreature();
        if (!creature)
        {
            handler->SendSysMessage("|cffff0000You can only capture creatures.|r");
            return true;
        }

        sSoulKeeper->AddGuardian(player, target);
        return true;
    }

    // .soul summon [index] - No arg: Open menu | With arg: Summon by index
    static bool HandleSummonCommand(ChatHandler* handler, Optional<uint32> index)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        // No argument = Open soul list menu
        if (!index.has_value() || index.value() == 0)
        {
            sSoulKeeper->ShowSoulList(player);
            return true;
        }

        // Load souls if not loaded
        if (sSoulKeeper->_caughtSouls[player->GetGUID().GetCounter()].empty())
        {
            sSoulKeeper->LoadSouls(player);
        }

        auto& souls = sSoulKeeper->_caughtSouls[player->GetGUID().GetCounter()];
        uint32 idx = index.value();

        if (idx > souls.size())
        {
            handler->PSendSysMessage("|cffff0000Invalid index. You have {} souls.|r", souls.size());
            return true;
        }

        // 1-based index → 0-based array access
        uint32 entry = souls[idx - 1].creatureEntry;
        sSoulKeeper->SummonGuardian(player, entry);
        return true;
    }

    // .soul dismiss - Dismiss active guardian
    static bool HandleDismissCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        sSoulKeeper->DismissGuardian(player);
        return true;
    }

    // .soul return - Recall guardian (despawn + 60s cooldown)
    // Use when guardian is chasing enemies too far away
    static bool HandleReturnCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        sSoulKeeper->ReturnGuardian(player);
        return true;
    }

    // .soul rename <name> - Rename currently summoned guardian
    static bool HandleRenameCommand(ChatHandler* handler, Tail name)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        std::string newName(name);
        if (newName.empty())
        {
            handler->SendSysMessage("Usage: .soul rename <new name>");
            return true;
        }

        sSoulKeeper->RenameGuardian(player, newName);
        return true;
    }

    // .soul search <partial_name> - Search souls by name (case-insensitive)
    // Shows up to 20 matches with their index for .soul summon
    static bool HandleSearchCommand(ChatHandler* handler, Tail searchTerm)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        std::string search(searchTerm);
        if (search.empty())
        {
            handler->SendSysMessage("Usage: .soul search <partial name>");
            return true;
        }

        // Load souls if not loaded
        if (sSoulKeeper->_caughtSouls[player->GetGUID().GetCounter()].empty())
        {
            sSoulKeeper->LoadSouls(player);
        }

        auto& souls = sSoulKeeper->_caughtSouls[player->GetGUID().GetCounter()];
        if (souls.empty())
        {
            handler->SendSysMessage("|cff888888No souls captured yet.|r");
            return true;
        }

        // Convert search term to lowercase for case-insensitive matching
        std::string searchLower = search;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

        // Search and collect matches
        std::vector<std::pair<uint32, std::string>> matches;  // index, name
        for (uint32 i = 0; i < souls.size(); ++i)
        {
            std::string nameLower = souls[i].customName;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            
            if (nameLower.find(searchLower) != std::string::npos)
            {
                matches.push_back({i + 1, souls[i].customName});  // 1-indexed for user
            }
        }

        if (matches.empty())
        {
            handler->PSendSysMessage("|cff888888No souls found matching '{}'|r", search);
            return true;
        }

        // Display results (limit to 20 to avoid spam)
        handler->PSendSysMessage("|cff00ff00Found {} soul(s) matching '{}':|r", matches.size(), search);
        uint32 shown = 0;
        for (const auto& match : matches)
        {
            if (shown >= 20)
            {
                handler->PSendSysMessage("|cff888888... and {} more. Refine your search.|r", matches.size() - 20);
                break;
            }
            handler->PSendSysMessage("  |cff00ff00[{}]|r {}", match.first, match.second);
            ++shown;
        }
        handler->SendSysMessage("|cff888888Use .soul summon <number> to summon.|r");
        return true;
    }
};

// =============================================================================
// UnitScript: Owner-Assist + Guardian Damage Scaling + Death Handling
// 
// OWNER-ASSIST BEHAVIOR:
// When a player with an active guardian enters combat or takes damage:
//   → Guardian automatically attacks the threat
//   → Uses native creature AI for abilities (not PetAI)
//   → Returns to follow when combat ends
//
// DAMAGE SCALING (Level-Bracket System):
// All guardian damage scales to owner's level and gear quality.
// Base DPS is determined by level bracket (e.g., 750 DPS at level 80).
// GearRatio = best of (meleeAP/expected, rangedAP/expected, spellPower/expected).
//
// SCALING FORMULAS:
//   → Melee:   DPS × attackTimeSeconds (creature's actual swing timer)
//   → Spells:  DPS × castTimeSeconds (instant = 1.0s)
//   → DoTs:    DPS × 0.15 × tickIntervalSeconds (15% extra DPS contribution)
//   → HoTs:    HPS × 0.15 × tickIntervalSeconds (15% extra HPS contribution)
//   → Shields: HPS × 3.0 (absorbs ~3 seconds of damage)
//   → Thorns:  DPS × 0.15 per proc
//
// GUARDIAN DEATH:
// When guardian dies → cleanup tracking
// =============================================================================
// =============================================================================
// UnitScript: Owner-Assist + Guardian Damage Scaling + Death Handling + AI Injection
// =============================================================================
class SoulKeeper_UnitScript : public UnitScript
{
public:
    SoulKeeper_UnitScript() : UnitScript("SoulKeeper_UnitScript", true, 
        { UNITHOOK_ON_UNIT_ENTER_COMBAT, UNITHOOK_ON_DAMAGE, UNITHOOK_ON_UNIT_DEATH,
          UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK,
          UNITHOOK_ON_UNIT_ENTER_EVADE_MODE, UNITHOOK_ON_UNIT_UPDATE,
          UNITHOOK_MODIFY_HEAL_RECEIVED, UNITHOOK_ON_AURA_APPLY }) { }

    // === GUARDIAN EVADES → Refresh stats + PRESERVE HP/MANA ===
    void OnUnitEnterEvadeMode(Unit* unit, uint8 /*evadeReason*/) override
    {
        Creature* creature = unit->ToCreature();
        if (!creature)
            return;

        ObjectGuid ownerGuid = creature->GetOwnerGUID();
        if (!ownerGuid || !ownerGuid.IsPlayer())
            return;

        uint32 ownerLow = ownerGuid.GetCounter();
        auto it = sSoulKeeper->_activeGuardians.find(ownerLow);
        if (it == sSoulKeeper->_activeGuardians.end() || it->second != creature->GetGUID())
            return;

        Player* owner = ObjectAccessor::GetPlayer(*creature, ownerGuid);
        if (!owner)
            return;

        // Save current HP/Mana percentages before rescaling
        float hpPct = (float)creature->GetHealth() / (float)creature->GetMaxHealth();
        float manaPct = 1.0f;
        if (creature->GetPowerType() == POWER_MANA && creature->GetMaxPower(POWER_MANA) > 0)
            manaPct = (float)creature->GetPower(POWER_MANA) / (float)creature->GetMaxPower(POWER_MANA);

        // Rescale guardian to current owner stats (picks up equipment changes)
        sSoulKeeper->ScaleGuardian(creature, owner);

        // Restore HP/Mana to same percentage (not full heal!)
        uint32 newHP = (uint32)(creature->GetMaxHealth() * hpPct);
        if (newHP < 1) newHP = 1;  // Prevent 0 HP
        creature->SetHealth(newHP);

        if (creature->GetPowerType() == POWER_MANA)
        {
            uint32 newMana = (uint32)(creature->GetMaxPower(POWER_MANA) * manaPct);
            creature->SetPower(POWER_MANA, newMana);
        }
    }

    // === AI INJECTION: Heals, Buffs, Dispels ===
    // Adds supportive spell behavior without replacing creature's native combat AI.
    // Uses per-guardian timer (not singleton!) for proper multi-guardian support.
    void OnUnitUpdate(Unit* unit, uint32 diff) override
    {
        Creature* creature = unit->ToCreature();
        if (!creature || !creature->IsAlive())
            return;

        // Only act if guardian is owned by player
        ObjectGuid ownerGuid = creature->GetOwnerGUID();
        if (!ownerGuid.IsPlayer()) 
            return;
        
        ObjectGuid guardianGuid = creature->GetGUID();
        
        // Check if it's our guardian
        uint32 ownerLow = ownerGuid.GetCounter();
        auto guardIt = sSoulKeeper->_activeGuardians.find(ownerLow);
        if (guardIt == sSoulKeeper->_activeGuardians.end() || guardIt->second != guardianGuid)
            return;

        // Per-guardian AI timer (not shared singleton!)
        auto& timerRef = sSoulKeeper->_guardianAITimer[guardianGuid];
        if (timerRef > diff)
        {
            timerRef -= diff;
            return;
        }
        timerRef = 1500; // Run every ~1.5 seconds

        // NOTE: Native AI buff spam prevention is now handled in core CombatAI.cpp
        // CombatAI::UpdateAI and JustEngagedWith now check HasAura() before casting self-buffs.

        // Don't interrupt existing actions or conflict with native AI
        // This ensures we play nice with SmartAI, ScriptedAI, CombatAI, etc.
        if (creature->HasUnitState(UNIT_STATE_CASTING | UNIT_STATE_STUNNED | 
                                   UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
            return;

        Player* owner = ObjectAccessor::GetPlayer(*creature, ownerGuid);
        if (!owner)
            return;

        CreatureTemplate const* cInfo = creature->GetCreatureTemplate();
        
        // =================================================================
        // PRIORITY 1: EMERGENCY HEALING (Critical HP)
        // Owner or self below 35% HP - immediate heal attempt
        // =================================================================
        bool ownerCritical = owner->GetHealthPct() < 35.0f;
        bool selfCritical = creature->GetHealthPct() < 35.0f;
        
        if (ownerCritical || selfCritical)
        {
            Unit* healTarget = ownerCritical ? (Unit*)owner : (Unit*)creature;
            if (TryCastHeal(creature, healTarget, cInfo))
            {
                timerRef = 2000;
                return;
            }
        }
        
        // =================================================================
        // PRIORITY 2: DISPEL (Remove CC/Debuffs from owner or self)
        // Check owner first (usually more important), then self
        // =================================================================
        if (HasDispellableDebuff(owner))
        {
            if (TryCastDispel(creature, owner, cInfo))
            {
                timerRef = 2500;
                return;
            }
        }
        // Self-dispel (guardian might be polymorphed, cursed, etc.)
        if (HasDispellableDebuff(creature))
        {
            if (TryCastDispel(creature, creature, cInfo))
            {
                timerRef = 2500;
                return;
            }
        }
        
        // =================================================================
        // PRIORITY 3: NORMAL HEALING (Low HP but not emergency)
        // Owner or self below 60% HP
        // =================================================================
        bool ownerLowHp = owner->GetHealthPct() < 60.0f;
        bool selfLowHp = creature->GetHealthPct() < 60.0f;
        
        if (ownerLowHp || selfLowHp)
        {
            Unit* healTarget = ownerLowHp ? (Unit*)owner : (Unit*)creature;
            if (TryCastHeal(creature, healTarget, cInfo))
            {
                timerRef = 2500;
                return;
            }
        }
        
        // =================================================================
        // PRIORITY 4: COMBAT BUFFS (like Bloodlust, Heroism, Battle Shout)
        // Cast important buffs during combat if not already active
        // =================================================================
        if (creature->IsInCombat())
        {
            // Buff owner first (they benefit most from Bloodlust etc.)
            if (TryCastBuff(creature, owner, cInfo))
            {
                timerRef = 2000;
                return;
            }
            // Then buff self
            if (TryCastBuff(creature, creature, cInfo))
            {
                timerRef = 2000;
                return;
            }
        }
        else
        {
            // =================================================================
            // OUT OF COMBAT BEHAVIOR: Rest and recuperate
            // Guardians should heal/buff like their wild counterparts would
            // =================================================================
            
            // OUT OF COMBAT HEALING: Heal owner or self if not at full HP
            // This mimics the "rest after combat" behavior players expect
            if (owner->GetHealthPct() < 95.0f)
            {
                if (TryCastHeal(creature, owner, cInfo))
                {
                    timerRef = 2000;
                    return;
                }
            }
            if (creature->GetHealthPct() < 95.0f)
            {
                if (TryCastHeal(creature, creature, cInfo))
                {
                    timerRef = 2000;
                    return;
                }
            }
            
            // OUT OF COMBAT BUFFS: Prep for next fight
            // Apply missing buffs to owner and self
            if (TryCastBuff(creature, owner, cInfo))
            {
                timerRef = 2000;
                return;
            }
            if (TryCastBuff(creature, creature, cInfo))
            {
                timerRef = 2000;
                return;
            }
        }
    }
    
private:
    // Check if unit has any dispellable negative aura (Magic, Curse, Disease, Poison)
    bool HasDispellableDebuff(Unit* unit)
    {
        for (auto const& pair : unit->GetAppliedAuras())
        {
            Aura const* aura = pair.second->GetBase();
            SpellInfo const* spellInfo = aura->GetSpellInfo();
            
            // Must be negative
            if (spellInfo->IsPositive()) continue;
            
            // Check dispel types
            uint32 dispelType = spellInfo->Dispel;
            if (dispelType == DISPEL_MAGIC || dispelType == DISPEL_CURSE ||
                dispelType == DISPEL_DISEASE || dispelType == DISPEL_POISON)
            {
                return true;
            }
        }
        return false;
    }
    
    // === HELPER: Get scaled mana cost by owner level ===
    // Prevents lvl 15 creature spells costing peanuts on lvl 77 guardians with huge mana pools.
    // Only applies if the spell actually costs mana (some spells are free).
    uint32 GetScaledManaCost(Creature* caster, SpellInfo const* spellInfo)
    {
        // If spell doesn't cost mana, return 0 (don't amplify free spells!)
        uint32 baseCost = spellInfo->CalcPowerCost(caster, spellInfo->GetSchoolMask());
        if (baseCost == 0 || spellInfo->PowerType != POWER_MANA)
            return 0;
        
        // Get owner level for bracket
        ObjectGuid ownerGuid = caster->GetOwnerGUID();
        if (!ownerGuid.IsPlayer())
            return baseCost; // No owner = use original cost
        
        auto scalingIt = sSoulKeeper->_guardianScaling.find(caster->GetGUID());
        if (scalingIt == sSoulKeeper->_guardianScaling.end())
            return baseCost;
        
        uint32 lvl = scalingIt->second.ownerLevel;
        
        // Level-bracketed fixed mana costs (% of max mana)
        // These are designed so spells feel meaningful but not crippling
        float manaPct;
        if (lvl <= 10)      { manaPct = 0.08f; }  // 8% of max mana
        else if (lvl <= 20) { manaPct = 0.07f; }  // 7%
        else if (lvl <= 30) { manaPct = 0.06f; }  // 6%
        else if (lvl <= 40) { manaPct = 0.05f; }  // 5%
        else if (lvl <= 50) { manaPct = 0.05f; }  // 5%
        else if (lvl <= 60) { manaPct = 0.04f; }  // 4%
        else if (lvl <= 70) { manaPct = 0.04f; }  // 4%
        else                { manaPct = 0.03f; }  // 3% at 80 (with big mana pools)
        
        return (uint32)(caster->GetMaxPower(POWER_MANA) * manaPct);
    }
    
    // Try to cast a heal spell on target
    bool TryCastHeal(Creature* caster, Unit* target, CreatureTemplate const* cInfo)
    {
        for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        {
            uint32 spellId = cInfo->spells[i];
            if (!spellId) continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo) continue;

            // Must be a positive healing spell
            if (!spellInfo->IsPositive() || spellInfo->IsPassive()) continue;
            if (!spellInfo->HasEffect(SPELL_EFFECT_HEAL) && 
                !spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT) &&
                !spellInfo->HasAura(SPELL_AURA_PERIODIC_HEAL)) continue;
            
            // Check cooldown
            if (caster->HasSpellCooldown(spellId)) continue;
            
            // Check range (use friendly range)
            float maxRange = spellInfo->GetMaxRange(true);
            if (maxRange > 0 && !caster->IsWithinDist(target, maxRange)) continue;
            
            // Check mana cost (scaled by level)
            uint32 scaledCost = GetScaledManaCost(caster, spellInfo);
            if (caster->GetPower(POWER_MANA) < scaledCost) continue;

            caster->CastSpell(target, spellId, false);
            
            // Add cooldown using spell's ACTUAL cooldown (no artificial minimum)
            // If spell has no cooldown, that's fine - the creature can spam it
            uint32 cooldown = spellInfo->GetRecoveryTime();
            if (cooldown > 0)
                caster->AddSpellCooldown(spellId, 0, cooldown);
            
            return true;
        }
        return false;
    }
    
    // Try to cast a dispel spell on target
    bool TryCastDispel(Creature* caster, Unit* target, CreatureTemplate const* cInfo)
    {
        for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        {
            uint32 spellId = cInfo->spells[i];
            if (!spellId) continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo) continue;

            // Must be a dispel spell
            if (!spellInfo->HasEffect(SPELL_EFFECT_DISPEL)) continue;
            
            // Check cooldown
            if (caster->HasSpellCooldown(spellId)) continue;
            
            // Check range
            float maxRange = spellInfo->GetMaxRange(true);
            if (maxRange > 0 && !caster->IsWithinDist(target, maxRange)) continue;
            
            // Check mana cost (scaled by level)
            uint32 scaledCost = GetScaledManaCost(caster, spellInfo);
            if (caster->GetPower(POWER_MANA) < scaledCost) continue;

            caster->CastSpell(target, spellId, false);
            
            // Add cooldown using spell's ACTUAL cooldown (no artificial minimum)
            uint32 cooldown = spellInfo->GetRecoveryTime();
            if (cooldown > 0)
                caster->AddSpellCooldown(spellId, 0, cooldown);
            
            return true;
        }
        return false;
    }
    
    // Try to cast a buff spell on target if they don't have it
    bool TryCastBuff(Creature* caster, Unit* target, CreatureTemplate const* cInfo)
    {
        for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        {
            uint32 spellId = cInfo->spells[i];
            if (!spellId) continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo) continue;

            // Must be a positive buff (not heal, not dispel, has duration, not passive)
            if (!spellInfo->IsPositive() || spellInfo->IsPassive()) continue;
            if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) || 
                spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT) ||
                spellInfo->HasEffect(SPELL_EFFECT_DISPEL)) continue;
            if (spellInfo->GetDuration() <= 0) continue;
            
            // IMMUNITY BUFFS: Only cast on SELF when ACTUALLY taking damage!
            // Don't waste the 60s cooldown just because combat started (mob still running to us).
            // CRITICAL: Immunity buffs should ONLY be cast on the one taking damage (self).
            // If target != caster, skip immunity spells entirely - don't waste the guardian's
            // defensive cooldown on someone else who isn't even being hit!
            bool isImmunity = spellInfo->HasAura(SPELL_AURA_SCHOOL_IMMUNITY) ||
                              spellInfo->HasAura(SPELL_AURA_DAMAGE_IMMUNITY) ||
                              spellInfo->HasAura(SPELL_AURA_MECHANIC_IMMUNITY) ||
                              spellInfo->HasAura(SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL);
            if (isImmunity)
            {
                // Immunity buffs are SELF-ONLY - don't cast on others
                if (target != caster)
                    continue;
                    
                // Check if WE (the caster/target) actually took damage recently
                auto damageIt = sSoulKeeper->_guardianLastDamage.find(caster->GetGUID());
                if (damageIt == sSoulKeeper->_guardianLastDamage.end())
                    continue; // No damage recorded yet, skip immunity
                    
                uint32 timeSinceDamage = getMSTimeDiff(damageIt->second, getMSTime());
                if (timeSinceDamage > 5000)
                    continue; // Damage was more than 5 seconds ago, skip immunity
            }
            
            // Skip if target already has this buff
            if (target->HasAura(spellId)) continue;
            
            // Check cooldown
            if (caster->HasSpellCooldown(spellId)) continue;
            
            // Check range
            float maxRange = spellInfo->GetMaxRange(true);
            if (maxRange > 0 && !caster->IsWithinDist(target, maxRange)) continue;
            
            // Check mana cost (scaled by level)
            uint32 scaledCost = GetScaledManaCost(caster, spellInfo);
            if (caster->GetPower(POWER_MANA) < scaledCost) continue;

            caster->CastSpell(target, spellId, false);
            
            // Determine cooldown based on spell type
            uint32 cooldown = spellInfo->GetRecoveryTime();
            
            // IMMUNITY AURAS: 60s minimum cooldown (already checked isImmunity above)
            if (isImmunity && cooldown < 60000)
                cooldown = 60000;
            
            // Add cooldown if any
            if (cooldown > 0)
                caster->AddSpellCooldown(spellId, 0, cooldown);
            
            return true;
        }
        return false;
    }

public:

    // === OWNER ENTERS COMBAT → Guardian assists ===
    void OnUnitEnterCombat(Unit* unit, Unit* victim) override
    {
        Player* player = unit->ToPlayer();
        if (!player || !victim)
            return;

        // Check if player has an active guardian
        uint32 playerGuidLow = player->GetGUID().GetCounter();
        auto it = sSoulKeeper->_activeGuardians.find(playerGuidLow);
        if (it == sSoulKeeper->_activeGuardians.end())
            return;

        Creature* guardian = ObjectAccessor::GetCreature(*player, it->second);
        if (!guardian || !guardian->IsAlive())
            return;

        // Guardian assists: attack owner's target
        if (guardian->CanCreatureAttack(victim) && !guardian->IsInCombatWith(victim))
        {
            guardian->AI()->AttackStart(victim);
        }
    }

    // === SPELL DAMAGE SCALING ===
    // Uses same level-bracket DPS formula as melee.
    // SpellDamage = DPS * castTimeSeconds (instant = 1.0s GCD equivalent)
    // Target: 25% of owner DPS at all levels
    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || damage <= 0)
            return;

        Creature* attackerCreature = attacker->ToCreature();
        if (!attackerCreature)
            return;

        auto scalingIt = sSoulKeeper->_guardianScaling.find(attackerCreature->GetGUID());
        if (scalingIt == sSoulKeeper->_guardianScaling.end())
            return;

        const auto& info = scalingIt->second;
        
        // === LEVEL BRACKET TARGET DPS ===
        // gearRatio is already pre-calculated and normalized in ScaleGuardian.
        // Now accounts for melee/ranged/spell expected values correctly!
        float targetDPS;
        uint32 lvl = info.ownerLevel;
        
        if (lvl <= 10)      { targetDPS = 1.5f; }
        else if (lvl <= 20) { targetDPS = 5.0f; }
        else if (lvl <= 30) { targetDPS = 12.0f; }
        else if (lvl <= 40) { targetDPS = 25.0f; }
        else if (lvl <= 50) { targetDPS = 45.0f; }
        else if (lvl <= 60) { targetDPS = 85.0f; }
        else if (lvl <= 70) { targetDPS = 175.0f; }
        else                { targetDPS = 750.0f; }
        
        // Use pre-calculated gear ratio directly (no more division)
        float totalDPS = targetDPS * info.gearRatio;
        
        // Spell damage = DPS * cast time
        // Instant spells (0ms cast) count as 1.0s matching GCD
        float castTimeSeconds = 1.0f;
        if (spellInfo)
        {
            float rawCastTime = spellInfo->CalcCastTime() / 1000.0f;
            if (rawCastTime > 0.0f)
                castTimeSeconds = rawCastTime;
            // else: instant spell, keep 1.0s default
        }
        
        damage = (int32)(totalDPS * castTimeSeconds);
    }

    // === HEAL VALUE SCALING (OUTGOING ONLY) ===
    // ONLY scales heals CAST BY our guardians, not heals RECEIVED by guardians!
    // Guardian CASTS heal → scale it based on guardian's level/gear
    // Player/NPC heals guardian → leave completely untouched (their healing, their power)
    void ModifyHealReceived(Unit* /*target*/, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
        if (!healer || heal == 0)
            return;

        // CRITICAL: NEVER touch heals cast by players!
        // This is the player's healing spell - it should heal for the player's amount.
        if (healer->GetTypeId() == TYPEID_PLAYER)
            return;

        // Must be a creature to be one of our guardians
        Creature* healerCreature = healer->ToCreature();
        if (!healerCreature)
            return;

        // Only scale if the HEALER (caster) is a Soul Keeper guardian
        // External heals (from other NPCs, bosses, etc.) are NOT our business
        auto scalingIt = sSoulKeeper->_guardianScaling.find(healerCreature->GetGUID());
        if (scalingIt == sSoulKeeper->_guardianScaling.end())
            return;

        const auto& info = scalingIt->second;
        
        // === LEVEL BRACKET TARGET HPS (same as DPS) ===
        float targetHPS;
        uint32 lvl = info.ownerLevel;
        
        if (lvl <= 10)      { targetHPS = 1.5f; }
        else if (lvl <= 20) { targetHPS = 5.0f; }
        else if (lvl <= 30) { targetHPS = 12.0f; }
        else if (lvl <= 40) { targetHPS = 25.0f; }
        else if (lvl <= 50) { targetHPS = 45.0f; }
        else if (lvl <= 60) { targetHPS = 85.0f; }
        else if (lvl <= 70) { targetHPS = 175.0f; }
        else                { targetHPS = 750.0f; }
        
        float totalHPS = targetHPS * info.gearRatio;
        
        // === HEAL SCALING ===
        // Direct heals: HPS * cast time (same as offensive spells)
        // HoTs: Target 15% HPS contribution using ACTUAL tick interval
        //       Formula: tickHeal = HPS * 0.15 * tickIntervalSeconds
        //       This ensures ALL HoTs contribute exactly 15% extra HPS regardless of tick speed
        
        bool isHoT = spellInfo && spellInfo->HasAura(SPELL_AURA_PERIODIC_HEAL);
        
        if (isHoT)
        {
            // Get tick interval from spellInfo (EffectAmplitude)
            float tickIntervalSeconds = 3.0f; // Default if not found
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_PERIODIC_HEAL)
                {
                    if (spellInfo->Effects[i].Amplitude > 0)
                        tickIntervalSeconds = spellInfo->Effects[i].Amplitude / 1000.0f;
                    break;
                }
            }
            // HoT contribution = 15% of HPS (reasonable supplemental healing)
            // per tick = HPS * 0.15 * tickInterval (normalizes all tick speeds)
            heal = (uint32)(totalHPS * 0.15f * tickIntervalSeconds);
            if (heal < 1) heal = 1; // Minimum 1 heal
        }
        else
        {
            // Direct heal: HPS * cast time
            // Instant spells (CalcCastTime = 0) count as 1.0s matching GCD
            float castTimeSeconds = 1.0f;
            if (spellInfo)
            {
                float rawCastTime = spellInfo->CalcCastTime() / 1000.0f;
                if (rawCastTime > 0.0f)
                    castTimeSeconds = rawCastTime;
            }
            heal = (uint32)(totalHPS * castTimeSeconds);
            if (heal < 1) heal = 1; // Minimum 1 heal
        }
    }

    // === AURA VALUE SCALING (OUTGOING ONLY) ===
    // ONLY scales auras CAST BY guardians, not auras received FROM external sources!
    // Scales fixed-value auras from guardians. Percentage-based auras are IGNORED.
    // Types scaled:
    //   - SPELL_AURA_SCHOOL_ABSORB / MANA_SHIELD → Shields (HPS * 3.0s worth = 3 seconds of healing)
    //   - SPELL_AURA_DAMAGE_SHIELD → Thorns (DPS * 0.15 = 15% of melee DPS per proc)
    void OnAuraApply(Unit* /*unit*/, Aura* aura) override
    {
        if (!aura)
            return;
        
        // Get the caster - must be a guardian (not player, not random NPC)
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;
        
        // CRITICAL: NEVER touch auras cast by players!
        // This is the player's buff/shield - it should work at the player's power level.
        if (caster->GetTypeId() == TYPEID_PLAYER)
            return;
        
        Creature* casterCreature = caster->ToCreature();
        if (!casterCreature)
            return;
        
        // Only scale if the CASTER is a Soul Keeper guardian
        auto scalingIt = sSoulKeeper->_guardianScaling.find(casterCreature->GetGUID());
        if (scalingIt == sSoulKeeper->_guardianScaling.end())
            return;
        
        const auto& info = scalingIt->second;
        
        // Get level bracket base value (same for DPS and HPS)
        float targetValue;
        uint32 lvl = info.ownerLevel;
        
        if (lvl <= 10)      { targetValue = 1.5f; }
        else if (lvl <= 20) { targetValue = 5.0f; }
        else if (lvl <= 30) { targetValue = 12.0f; }
        else if (lvl <= 40) { targetValue = 25.0f; }
        else if (lvl <= 50) { targetValue = 45.0f; }
        else if (lvl <= 60) { targetValue = 85.0f; }
        else if (lvl <= 70) { targetValue = 175.0f; }
        else                { targetValue = 750.0f; }
        
        float totalValue = targetValue * info.gearRatio;
        
        // Check each effect for scalable aura types
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            AuraEffect* effect = aura->GetEffect(i);
            if (!effect)
                continue;
            
            AuraType auraType = effect->GetAuraType();
            int32 newAmount = 0;
            
            switch (auraType)
            {
                // === ABSORB SHIELDS: Worth 3 seconds of HPS ===
                // Shields are "stored healing" - should absorb meaningful damage
                // At level 80 with total DPS ~750, shield = 750 * 3 = 2250 absorb
                // This is about one big hit worth of protection
                case SPELL_AURA_SCHOOL_ABSORB:
                case SPELL_AURA_MANA_SHIELD:
                    newAmount = (int32)(totalValue * 3.0f);
                    break;
                
                // === DAMAGE SHIELD (Thorns): 15% of DPS per proc ===
                // Thorns is "free" damage on being hit
                // Balanced lower because it requires no GCD and stacks with everything
                case SPELL_AURA_DAMAGE_SHIELD:
                    newAmount = (int32)(totalValue * 0.15f);
                    break;
                
                default:
                    continue; // Skip unhandled aura types
            }
            
            // Apply the scaled amount (minimum 1)
            if (newAmount < 1) newAmount = 1;
            effect->ChangeAmount(newAmount, true, false);
        }
    }

    // === DOT DAMAGE SCALING ===
    // DoTs add supplemental damage on TOP of auto-attacks.
    // Target: 15% extra DPS from DoTs using ACTUAL tick interval.
    // Formula: tickDamage = DPS * 0.15 * tickIntervalSeconds
    // This ensures ALL DoTs contribute exactly 15% extra DPS regardless of tick speed.
    // NOTE: This hook is confusingly called for BOTH DoTs AND HoTs! Must skip heals.
    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || damage == 0)
            return;

        // SKIP HEALS: This hook is called for HoTs too (confusing naming!)
        // HoTs are handled by ModifyHealReceived instead
        if (spellInfo && spellInfo->HasAura(SPELL_AURA_PERIODIC_HEAL))
            return;

        Creature* attackerCreature = attacker->ToCreature();
        if (!attackerCreature)
            return;

        auto scalingIt = sSoulKeeper->_guardianScaling.find(attackerCreature->GetGUID());
        if (scalingIt == sSoulKeeper->_guardianScaling.end())
            return;

        const auto& info = scalingIt->second;
        
        // === LEVEL BRACKET TARGET DPS ===
        float targetDPS;
        uint32 lvl = info.ownerLevel;
        
        if (lvl <= 10)      { targetDPS = 1.5f; }
        else if (lvl <= 20) { targetDPS = 5.0f; }
        else if (lvl <= 30) { targetDPS = 12.0f; }
        else if (lvl <= 40) { targetDPS = 25.0f; }
        else if (lvl <= 50) { targetDPS = 45.0f; }
        else if (lvl <= 60) { targetDPS = 85.0f; }
        else if (lvl <= 70) { targetDPS = 175.0f; }
        else                { targetDPS = 750.0f; }
        
        float totalDPS = targetDPS * info.gearRatio;
        
        // Get tick interval from spellInfo (EffectAmplitude)
        float tickIntervalSeconds = 3.0f; // Default if not found
        if (spellInfo)
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                // Find the periodic damage/leech effect
                uint32 auraName = spellInfo->Effects[i].ApplyAuraName;
                if (auraName == SPELL_AURA_PERIODIC_DAMAGE || 
                    auraName == SPELL_AURA_PERIODIC_LEECH ||
                    auraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
                {
                    if (spellInfo->Effects[i].Amplitude > 0)
                        tickIntervalSeconds = spellInfo->Effects[i].Amplitude / 1000.0f;
                    break;
                }
            }
        }
        
        // DoT contribution = 15% of DPS (reasonable supplemental damage)
        // per tick = DPS * 0.15 * tickInterval (normalizes all tick speeds)
        damage = (uint32)(totalDPS * 0.15f * tickIntervalSeconds);
        if (damage < 1) damage = 1; // Minimum 1 damage
    }

    // === MELEE DAMAGE: No hook scaling needed ===
    // Melee damage is already set correctly in ScaleGuardian via SetStatFloatValue

    // === DAMAGE HANDLING: Guardian assists/defends owner ===
    // 1. When owner DEALS damage → guardian attacks the same target (ranged initiation)
    // 2. When owner TAKES damage → guardian attacks the attacker (defense)
    void OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/) override
    {
        if (!attacker || !victim)
            return;

        // === CASE 1: Owner ATTACKS something → Guardian assists ===
        Player* attackingPlayer = attacker->ToPlayer();
        if (attackingPlayer)
        {
            uint32 playerGuidLow = attackingPlayer->GetGUID().GetCounter();
            auto it = sSoulKeeper->_activeGuardians.find(playerGuidLow);
            if (it != sSoulKeeper->_activeGuardians.end())
            {
                Creature* guardian = ObjectAccessor::GetCreature(*attackingPlayer, it->second);
                if (guardian && guardian->IsAlive())
                {
                    // Guardian assists: attack owner's target
                    if (guardian->CanCreatureAttack(victim) && !guardian->IsInCombatWith(victim))
                    {
                        guardian->AI()->AttackStart(victim);
                    }
                }
            }
        }

        // === CASE 2: Owner TAKES damage → Guardian defends + Track damage time ===
        Player* victimPlayer = victim->ToPlayer();
        if (victimPlayer)
        {
            uint32 playerGuidLow = victimPlayer->GetGUID().GetCounter();
            auto it = sSoulKeeper->_activeGuardians.find(playerGuidLow);
            if (it != sSoulKeeper->_activeGuardians.end())
            {
                Creature* guardian = ObjectAccessor::GetCreature(*victimPlayer, it->second);
                if (guardian && guardian->IsAlive())
                {
                    // TRACK DAMAGE: Owner took damage, record timestamp on guardian
                    // This enables defensive cooldowns (immunity buffs) to trigger
                    sSoulKeeper->_guardianLastDamage[guardian->GetGUID()] = getMSTime();
                    
                    // Guardian defends: attack whoever hit the owner
                    if (guardian->CanCreatureAttack(attacker) && !guardian->IsInCombatWith(attacker))
                    {
                        guardian->AI()->AttackStart(attacker);
                    }
                }
            }
        }
        
        // === CASE 3: Guardian TAKES damage → Track damage time ===
        Creature* victimCreature = victim->ToCreature();
        if (victimCreature)
        {
            // Check if this creature is a tracked guardian
            auto scalingIt = sSoulKeeper->_guardianScaling.find(victimCreature->GetGUID());
            if (scalingIt != sSoulKeeper->_guardianScaling.end())
            {
                // Guardian took damage, record timestamp
                sSoulKeeper->_guardianLastDamage[victimCreature->GetGUID()] = getMSTime();
            }
        }
    }

    // === GUARDIAN DIES → Cooldown triggered ===
    void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
    {
        Creature* creature = unit->ToCreature();
        if (!creature)
            return;

        // Only care about creatures with a player owner
        ObjectGuid ownerGuid = creature->GetOwnerGUID();
        if (!ownerGuid || !ownerGuid.IsPlayer())
            return;

        // Check if this is a tracked guardian
        uint32 ownerLow = ownerGuid.GetCounter();
        auto it = sSoulKeeper->_activeGuardians.find(ownerLow);
        if (it != sSoulKeeper->_activeGuardians.end() && it->second == creature->GetGUID())
        {
            sSoulKeeper->OnGuardianDeath(creature);
        }
    }
};

// =============================================================================
// Loader
// =============================================================================
void Addmod_soul_keeperScripts()
{
    new SoulKeeper_PlayerScript();
    new SoulKeeper_CommandScript();
    new SoulKeeper_UnitScript();
}
