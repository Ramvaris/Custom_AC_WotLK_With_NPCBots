/*
 * Soul Keeper Module - Implementation
 * Capture creature souls and summon them as guardians
 * Pure C++ - No Lua dependency
 * 
 * COMMANDS:
 *   .soul absorb         - Capture target (DEAD creatures only)
 *   .soul dismiss        - Dismiss active guardian (60s cooldown)
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
 *   - Guardian death triggers 60s cooldown
 * 
 * OWNER-ASSIST BEHAVIOR (via UnitScript hooks):
 *   - OnUnitEnterCombat: Guardian attacks when owner attacks something
 *   - OnDamage: Guardian defends when owner takes damage
 *   - Guardian uses its native AI for spells/abilities during combat
 *   - REACT_DEFENSIVE prevents guardian from pulling extra mobs
 */

#include "SoulKeeper.h"
#include "Chat.h"
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

    QueryResult result = CharacterDatabase.Query(
        "SELECT creature_entry, custom_name, display_id, scale_factor FROM character_soul_keeper WHERE owner_guid = {}",
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

    if (souls.empty())
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
            "|cff888888No souls captured yet. Target a dead enemy and use .soul absorb|r", 
            GOSSIP_SENDER_MAIN, SOUL_ACTION_CLOSE);
    }
    else
    {
        // === PAGINATION MATH ===
        uint32 totalSouls = (uint32)souls.size();
        uint32 totalPages = (totalSouls + SOULS_PER_PAGE - 1) / SOULS_PER_PAGE;
        uint32 startIndex = page * SOULS_PER_PAGE;
        uint32 endIndex = std::min(startIndex + SOULS_PER_PAGE, totalSouls);

        // Page header
        if (totalPages > 1)
        {
            std::string pageInfo = "|cff888888Page " + std::to_string(page + 1) + "/" + std::to_string(totalPages) + 
                                   " (" + std::to_string(totalSouls) + " souls)|r";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, pageInfo, GOSSIP_SENDER_MAIN, SOUL_ACTION_CLOSE);
        }

        // Soul list for current page
        for (uint32 i = startIndex; i < endIndex; ++i)
        {
            const SoulData& soul = souls[i];
            std::string icon = GetCreatureIconString(soul.displayId);
            // Display as 1-indexed for user, store actual array index in action
            std::string label = "|cff00ff00[" + std::to_string(i + 1) + "]|r " + icon + " " + soul.customName;
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, label, GOSSIP_SENDER_MAIN, SOUL_ACTION_SUMMON_BASE + i);
        }

        // === NAVIGATION BUTTONS ===
        if (page > 0)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
                "|TInterface\\Icons\\Ability_Druid_Dash_Orange:20:20:-2:0|t << Previous Page", 
                GOSSIP_SENDER_MAIN, SOUL_ACTION_PREV_PAGE);
        }
        if (page < totalPages - 1)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
                "|TInterface\\Icons\\Ability_Druid_Dash:20:20:-2:0|t Next Page >>", 
                GOSSIP_SENDER_MAIN, SOUL_ACTION_NEXT_PAGE);
        }
    }

    // Absorb target option
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, 
        "|TInterface\\Icons\\Spell_Shadow_SoulGem:20:20:-2:0|t Capture Target's Soul", 
        GOSSIP_SENDER_MAIN, SOUL_ACTION_ABSORB);

    // Dismiss option (only if guardian is active)
    if (HasActiveGuardian(player))
    {
        AddGossipItemFor(player, GOSSIP_ICON_TALK, 
            "|TInterface\\Icons\\Spell_Holy_Dispel:20:20:-2:0|t Dismiss Guardian", 
            GOSSIP_SENDER_MAIN, SOUL_ACTION_DISMISS);
    }

    // Close
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, 
        "|TInterface\\Icons\\Misc_ArrowLeft:20:20:-2:0|t Close", 
        GOSSIP_SENDER_MAIN, SOUL_ACTION_CLOSE);

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

        case SOUL_ACTION_DISMISS:
            DismissGuardian(player);
            ShowSoulList(player, _currentGossipPage[playerGuid]);
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

        case SOUL_ACTION_ABSORB:
        {
            Unit* target = player->GetSelectedUnit();
            if (!target || !target->ToCreature())
            {
                ChatHandler(player->GetSession()).SendSysMessage("|cffff0000You must target a creature.|r");
                ShowSoulList(player, _currentGossipPage[playerGuid]);
                return true;
            }

            AddGuardian(player, target);
            ShowSoulList(player, _currentGossipPage[playerGuid]);
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
    newSoul.scaleFactor   = creature->GetObjectScale();

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

    // === Enable auto-attack for ALL creatures ===
    guardian->SetAttackTime(BASE_ATTACK, 2000);
    guardian->SetAttackTime(OFF_ATTACK, 2000);
    guardian->SetAttackTime(RANGED_ATTACK, 2000);

    // === Apply our custom scaling on TOP of InitStatsForLevel ===
    ScaleGuardian(guardian, player);

    // === Follow Master (RIGHT side to avoid overlapping Hunter/Warlock pets on LEFT) ===
    // CRITICAL: Must also set m_followAngle via SetFollowAngle(), otherwise when AI or
    // other code calls MoveFollow(owner, dist, GetFollowAngle()), it uses the default LEFT!
    // Guardian inherits from Minion, so we can cast directly.
    if (summon->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
    {
        ((Minion*)summon)->SetFollowAngle(-PET_FOLLOW_ANGLE);
    }
    guardian->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, -PET_FOLLOW_ANGLE);

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
    // Use the highest of: MeleeAP, RangedAP, SpellPower as "masterStat"
    // This determines the guardian's damage regardless of the original creature's stats
    float meleeAP  = owner->GetTotalAttackPowerValue(BASE_ATTACK);
    float rangedAP = owner->GetTotalAttackPowerValue(RANGED_ATTACK);
    float maxSP    = 0.0f;
    
    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
    {
        float sp = (float)owner->SpellBaseDamageBonusDone((SpellSchoolMask)(1 << i));
        if (sp > maxSP) maxSP = sp;
    }

    float masterStat = std::max({meleeAP, rangedAP, maxSP});

    // === UNIFIED DPS FORMULA ===
    // All damage uses the same BASE DPS formula, then adjusted by attack/cast time.
    // Formula: DPS = level * 2 + stat * 0.52
    // Target: 20% of owner DPS at endgame (level 80, 3500 SP, 10K owner DPS = 2000 guardian DPS)
    // Math: 80*2 + 3500*0.52 = 160 + 1820 = 1980 DPS ≈ 20%
    // While leveling the ratio will be higher (~40%), which is acceptable.
    float baseDPS  = ownerLevel * 2.0f;
    float statDPS  = masterStat * 0.52f;
    float totalDPS = baseDPS + statDPS;

    // === MELEE DAMAGE ===
    // Use ACTUAL creature attack speed (not hardcoded 2.0s)
    // Damage per hit = DPS * attackTimeSeconds
    float attackTime = (float)guardian->GetAttackTime(BASE_ATTACK);
    if (attackTime <= 0.0f) attackTime = 2000.0f;  // Default if not set (0 = invalid)

    // Damage per Hit = DPS * (attackTime / 1000)
    float damagePerHit = totalDPS * (attackTime / 1000.0f);
    
    // === DAMAGE SCALING (Guardian-compatible) ===
    // Guardian::UpdateDamagePhysical calculates: (BASE_VALUE + AP/14*att_speed + weapon_damage)
    // To avoid double-counting, we:
    // 1. Set AP to 0 (eliminate AP contribution)
    // 2. Set UNIT_MOD_DAMAGE_MAINHAND BASE_VALUE to 0
    // 3. Set weapon damage to our target (this becomes the final damage)
    guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_DAMAGE_MAINHAND, BASE_VALUE, 0.0f);
    guardian->SetStatFlatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, 0.0f);
    
    // Weapon damage is the SOLE source of our melee damage
    guardian->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, damagePerHit * 0.9f);
    guardian->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, damagePerHit * 1.1f);
    guardian->UpdateAttackPowerAndDamage(false);
    guardian->UpdateDamagePhysical(BASE_ATTACK);
    
    // Verify final values are set (safety net)
    guardian->SetStatFloatValue(UNIT_FIELD_MINDAMAGE, damagePerHit * 0.9f);
    guardian->SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, damagePerHit * 1.1f);
    guardian->SetMaxHealth(finalHealth);
    guardian->SetHealth(finalHealth);

    // === SPELL DAMAGE SCALING INFO ===
    // Store owner's masterStat for spell damage scaling in hooks
    // Guardian spells will scale directly from owner's stats using same DPS formula
    GuardianScalingInfo scalingInfo;
    scalingInfo.baseMultiplier = masterStat;  // Owner's power stat (highest of AP/SP)
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
            // Clean up scaling data
            _guardianScaling.erase(guardian->GetGUID());
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
            // Clean up scaling data
            _guardianScaling.erase(guardian->GetGUID());
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
        if (menu_id == SOUL_KEEPER_GOSSIP_MENU_ID || sender == GOSSIP_SENDER_MAIN)
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
// DAMAGE SCALING:
// All guardian damage uses UNIFIED DPS formula based on OWNER'S STATS:
//   → DPS = ownerLevel * 2 + masterStat * 0.52
//   → Melee: DPS * ACTUAL attack speed (reads creature's real swing timer)
//   → Spells: DPS * castTime (instant = 1.0s GCD equivalent)
//   → DoTs: 30% of DPS per tick (reduced because they STACK with auto-attacks)
//   → Target: 20% of owner DPS at endgame (level 80, 3500 SP, 10K owner DPS)
//   → While leveling the ratio is higher (~40%), which is acceptable
//
// GUARDIAN DEATH:
// When guardian dies → cleanup tracking
// =============================================================================
class SoulKeeper_UnitScript : public UnitScript
{
public:
    SoulKeeper_UnitScript() : UnitScript("SoulKeeper_UnitScript", true, 
        { UNITHOOK_ON_UNIT_ENTER_COMBAT, UNITHOOK_ON_DAMAGE, UNITHOOK_ON_UNIT_DEATH,
          UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) { }

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
    // Direct damage spells use the same DPS formula as melee.
    // DPS = level * 2 + stat * 0.52
    // SpellDamage = DPS * castTimeSeconds (instant = 1.0s GCD equivalent)
    // Target: 20% of owner DPS at endgame
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
        
        // Base DPS formula (same coefficient as melee)
        float baseDPS = info.ownerLevel * 2.0f;
        float statDPS = info.baseMultiplier * 0.52f;
        float totalDPS = baseDPS + statDPS;
        
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

    // === DOT DAMAGE SCALING ===
    // DoTs use REDUCED damage because they STACK with auto-attacks!
    // If auto-attack = 20% and DoT = 20%, combined = 40% (too high)
    // Solution: DoT per tick = 30% of the base DPS (adds ~6% total DPS on top of autos)
    // This makes DoTs useful but not overpowered.
    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
    {
        if (!attacker || !target || damage == 0)
            return;

        Creature* attackerCreature = attacker->ToCreature();
        if (!attackerCreature)
            return;

        auto scalingIt = sSoulKeeper->_guardianScaling.find(attackerCreature->GetGUID());
        if (scalingIt == sSoulKeeper->_guardianScaling.end())
            return;

        const auto& info = scalingIt->second;
        
        // Same base DPS formula
        float baseDPS = info.ownerLevel * 2.0f;
        float statDPS = info.baseMultiplier * 0.52f;
        float totalDPS = baseDPS + statDPS;
        
        // DoT tick = 30% of DPS (reduced because it stacks with autos)
        // Typical 3s tick DoT with 5 ticks = 1.5x DPS total = ~6% extra on top of 20% autos
        damage = (uint32)(totalDPS * 0.30f);
    }

    // === MELEE DAMAGE: No hook scaling needed ===
    // Melee damage is already set correctly in ScaleGuardian via SetStatFloatValue
    // We removed the melee hook since it was causing double-scaling issues

    // === DAMAGE HANDLING: Guardian defends owner ===
    // When owner takes damage, guardian attacks the attacker
    void OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/) override
    {
        if (!attacker || !victim)
            return;

        // Only care about player victims
        Player* player = victim->ToPlayer();
        if (!player)
            return;

        // Check if player has an active guardian
        uint32 playerGuidLow = player->GetGUID().GetCounter();
        auto it = sSoulKeeper->_activeGuardians.find(playerGuidLow);
        if (it == sSoulKeeper->_activeGuardians.end())
            return;

        Creature* guardian = ObjectAccessor::GetCreature(*player, it->second);
        if (!guardian || !guardian->IsAlive())
            return;

        // Guardian defends: attack whoever hit the owner
        if (guardian->CanCreatureAttack(attacker) && !guardian->IsInCombatWith(attacker))
        {
            // Stop current target if switching to defend owner
            if (guardian->GetVictim() != attacker)
            {
                guardian->AI()->AttackStart(attacker);
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
