/*
 * mod-custom-ramvaris — Private server customizations for Ramvaris' WotLK 3.3.5a server
 *
 * DEAD MODULE for cloners: All features require custom DBC entries and/or custom DB data.
 * Safe to delete if you don't have Ramvaris' custom server data.
 */

#ifndef LOADER_CUSTOM_RAMVARIS_H
#define LOADER_CUSTOM_RAMVARIS_H

// Spell Scripts
void AddSC_spell_human_stoneform();

// NPC Scripts (Lilly helper NPC — full gossip + summon flavor)
void AddSC_npc_lilly_gossip();

// NPC flavor text is NOT registered here — it's a centralized function
// called from the .special command handler (see npc_summon_flavor.h).

// Player Scripts (login spell grants, profession unlocks, racial adjustments)
void AddSC_player_login_grants();

// World Scripts (Dark Azeroth — biome-aware bad weather system)
void AddSC_dark_azeroth_weather();

// Command Scripts (.special, .mountup, .guardianscale, .petscale)
void AddSC_custom_commands();

// Random Enchants (Diablo-style class-specific, merged from mod-random-enchants)
void AddSC_random_enchants();

// Smart Wandering Bots (zone-aware dynamic bot spawning, core-patched BotDataMgr)
void AddSC_smart_wandering_bots();

// All-Class Equipment (startup hook: re-apply AllowableClass=-1, open class quests)
void AddSC_all_class_equip();

#endif // LOADER_CUSTOM_RAMVARIS_H
