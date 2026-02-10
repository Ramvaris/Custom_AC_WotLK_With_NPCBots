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
void AddSC_player_mend_pet_scale();

// World Scripts
void AddSC_dark_azeroth_weather();

// Command Scripts
void AddSC_custom_commands();

void Addmod_custom_ramvarisScripts()
{
    AddSC_spell_human_stoneform();
    AddSC_npc_summon_flavor();
    AddSC_npc_lilly_gossip();
    AddSC_player_login_grants();
    AddSC_player_mend_pet_scale();
    AddSC_dark_azeroth_weather();
    AddSC_custom_commands();
}
