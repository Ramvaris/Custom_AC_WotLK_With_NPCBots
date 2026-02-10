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

// NPC Scripts (summon flavor text for 7 custom NPCs)
void AddSC_npc_summon_flavor();

// NPC Scripts (Lilly helper NPC — full gossip + summon flavor)
void AddSC_npc_lilly_gossip();

// Player Scripts (login spell grants, profession unlocks, racial adjustments)
void AddSC_player_login_grants();

// World Scripts (Dark Azeroth — biome-aware bad weather system)
void AddSC_dark_azeroth_weather();

// Command Scripts (.special, .mountup, .guardianscale, .petscale)
void AddSC_custom_commands();

#endif // LOADER_CUSTOM_RAMVARIS_H
