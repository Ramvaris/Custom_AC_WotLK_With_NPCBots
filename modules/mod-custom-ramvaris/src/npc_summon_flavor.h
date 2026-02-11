/*
 * NPC Summon Flavor Text — Centralized function for random summon sayings
 * Called from the .special gossip handler after SummonCreature().
 *
 * Architecture note: In the original Lua, creature event hooks were additive —
 * a creature could have a module ScriptName AND a Lua OnSummon callback.
 * In C++, a creature can only have ONE ScriptName. Using per-creature CreatureAI
 * scripts for flavor text would overwrite the functional ScriptNames from other
 * modules (instance_reset, npc_reagent_banker, npc_warlock_pet_renamer, etc.).
 * This centralized function avoids that conflict entirely.
 */

#ifndef NPC_SUMMON_FLAVOR_H
#define NPC_SUMMON_FLAVOR_H

class Creature;
class WorldObject;

/// Triggers entry-based flavor text on a freshly summoned custom NPC.
/// Config-gated: CustomRamvaris.Enable + CustomRamvaris.NpcFlavorText.Enable.
/// Lilly (299900) is NOT handled here — her flavor text lives in npc_lilly_gossip.cpp.
void DoSummonFlavorText(Creature* creature, WorldObject* summoner);

#endif // NPC_SUMMON_FLAVOR_H
