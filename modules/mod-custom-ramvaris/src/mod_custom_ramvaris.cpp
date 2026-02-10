/*
 * mod-custom-ramvaris — Module Loader
 *
 * Private server customizations for Ramvaris' WotLK 3.3.5a server.
 * All features are config-gated: global toggle ON, sub-features OFF by default.
 * Cloners: This module is safe to delete — it does nothing without custom server data.
 */

#include "ScriptMgr.h"

// Spell Scripts
void AddSC_spell_human_stoneform();

// NPC Scripts
void AddSC_npc_summon_flavor();
void AddSC_npc_lilly_gossip();

// Player Scripts
void AddSC_player_login_grants();

// World Scripts
void AddSC_dark_azeroth_weather();

// Command Scripts
void AddSC_custom_commands();

// Random Enchants (merged from mod-random-enchants)
void AddSC_random_enchants();

// Smart Wandering Bots (zone-aware, core-patched BotDataMgr)
void AddSC_smart_wandering_bots();

void Addmod_custom_ramvarisScripts()
{
    AddSC_spell_human_stoneform();
    AddSC_npc_summon_flavor();
    AddSC_npc_lilly_gossip();
    AddSC_player_login_grants();
    AddSC_dark_azeroth_weather();
    AddSC_custom_commands();
    AddSC_random_enchants();
    AddSC_smart_wandering_bots();
}
