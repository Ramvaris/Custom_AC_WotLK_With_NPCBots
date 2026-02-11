/*
 * NPC Summon Flavor Text — Centralized flavor text for custom NPCs
 * Config: CustomRamvaris.NpcFlavorText.Enable
 *
 * Called from the .special gossip handler after SummonCreature().
 * NOT individual CreatureAI scripts — a creature can only have ONE ScriptName,
 * and these NPCs already have functional ScriptNames from other modules
 * (npc_reagent_banker, npc_warlock_pet_renamer, npc_transmogrifier).
 *
 * Handles 6 custom NPCs (Lilly is in npc_lilly_gossip.cpp):
 *   290011 - Ling (Reagent Bank)        — ScriptName: npc_reagent_banker
 *   299902 - Ashari (Vendor)            — vendor via npcflag
 *   299901 - Lord Squeak (Emblems)      — quest giver via npcflag
 *   299903 - Ciel (Mounts)              — vendor via npcflag
 *   200002 - Rename (Pet Renamer)       — ScriptName: npc_warlock_pet_renamer
 *   190011 - Warpweaver (Transmog)      — ScriptName: npc_transmogrifier
 */

#include "npc_summon_flavor.h"
#include "Creature.h"
#include "Config.h"
#include "ObjectGuid.h"
#include "Random.h"

static bool IsFlavorEnabled()
{
    return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
           sConfigMgr->GetOption<bool>("CustomRamvaris.NpcFlavorText.Enable", false);
}

// ============================================================================
// Per-entry flavor text dispatchers
// ============================================================================

static void FlavorLing(Creature* me, std::string const& name)
{
    uint32 choice = urand(1, 12);
    switch (choice)
    {
        case 1:  me->Say("What do you want, " + name + "?", LANG_UNIVERSAL); break;
        case 2:  me->Say("What if you do my job for a day, " + name + "?", LANG_UNIVERSAL); break;
        case 3:  me->Say("Let me guess, you want to store some reagents?", LANG_UNIVERSAL); break;
        case 4:
            me->Say("Maybe I am the one saving Azeroth. I mean, I carry all this stuff around.", LANG_UNIVERSAL);
            me->TextEmote("Ling sighs.");
            break;
        case 5:  me->Say("It's always the same thing...", LANG_UNIVERSAL); break;
        case 6:  me->Say("You know, my back is killing me. This bank isn't weightless!", LANG_UNIVERSAL); break;
        case 7:  me->Say("I was having a lovely dream about a world without adventurers... then you called.", LANG_UNIVERSAL); break;
        case 8:  me->Say("Can we make this quick? I have... absolutely nothing else to do, but I'd like to pretend.", LANG_UNIVERSAL); break;
        case 9:  me->Say("Did you wash your hands? I don't want sticky fingerprints on the vault.", LANG_UNIVERSAL); break;
        case 10:
            me->Say("I am sworn to carry your burdens... literally.", LANG_UNIVERSAL);
            me->TextEmote("Ling glares at you.");
            break;
        case 11: me->Say("Do you have any idea what time it is? ...Actually, neither do I. It's always 'now' here.", LANG_UNIVERSAL); break;
        default:
            me->Say("Here I am. Again. Hooray.", LANG_UNIVERSAL);
            me->TextEmote("Ling rolls her eyes.");
            break;
    }
}

static void FlavorAshari(Creature* me, std::string const& name)
{
    uint32 choice = urand(1, 12);
    switch (choice)
    {
        case 1:  me->Say("Oh, come on! I was just polishing my horns!", LANG_UNIVERSAL); break;
        case 2:  me->Say("Hey " + name + "! You really like me, eh?", LANG_UNIVERSAL); break;
        case 3:
            me->Say("Do you want to buy? To trade? I am good at impersonating Draenei.", LANG_UNIVERSAL);
            me->TextEmote("Ashari giggles.");
            break;
        case 4:  me->Say("Ashari has wares, if you have coin! Just heard that from a really strange cat guy.", LANG_UNIVERSAL); break;
        case 5:  me->Say("Ten thousand gold coins... ten thou... oh, hey " + name + "!", LANG_UNIVERSAL); break;
        case 6:
            me->Say("No refunds! ...unless you are very, very persuasive.", LANG_UNIVERSAL);
            me->TextEmote("Ashari winks.");
            break;
        case 7:  me->Say("I found this item... fell off a wagon. You want it?", LANG_UNIVERSAL); break;
        case 8:  me->Say("Welcome to Ashari's Emporium of Wonders! And by wonders, I mean stuff I found.", LANG_UNIVERSAL); break;
        case 9:  me->Say("You look like you have too much gold. Let me help you with that burden.", LANG_UNIVERSAL); break;
        case 10: me->Say("Don't tell the others, but you get the 'special friend' discount... which is 0% off!", LANG_UNIVERSAL); break;
        case 11: me->Say("I saw a goblin earlier. Tried to sell me my own shoes! Can you believe the nerve?", LANG_UNIVERSAL); break;
        default:
            me->Say("Is that a mirror in your pocket? Because I can see myself in your gold.", LANG_UNIVERSAL);
            me->TextEmote("Ashari grins widely.");
            break;
    }
}

static void FlavorSqueak(Creature* me)
{
    uint32 choice = urand(1, 12);
    switch (choice)
    {
        case 1:  me->Say("Squeak! I mean... Yes? What is it?", LANG_UNIVERSAL); break;
        case 2:  me->Say("I am a very busy Lord! Make it quick!", LANG_UNIVERSAL); break;
        case 3:  me->Say("You want my influence? It will cost you... cheese! I mean, emblems!", LANG_UNIVERSAL); break;
        case 4:  me->Say("Do not look at my tail! Look at my monocle! I am civilized!", LANG_UNIVERSAL); break;
        case 5:
            me->Say("Yes-yes? You want trade? Shiny emblems for shiny armor?", LANG_UNIVERSAL);
            me->TextEmote("Lord Squeak rubs his paws together.");
            break;
        case 6:  me->Say("I smell... success! And maybe old cheddar.", LANG_UNIVERSAL); break;
        case 7:
            me->Say("Quickly now! The cat-beast might be watching!", LANG_UNIVERSAL);
            me->TextEmote("Lord Squeak looks around nervously.");
            break;
        case 8:  me->Say("You bring emblems? Good-good. I give gear. Fair trade!", LANG_UNIVERSAL); break;
        case 9:  me->Say("My prices are the best in the under-city! I mean... the city!", LANG_UNIVERSAL); break;
        case 10: me->Say("No traps? Good. We talk business.", LANG_UNIVERSAL); break;
        case 11: me->Say("I am nobility! Treat me with respect or I knaw on your boots!", LANG_UNIVERSAL); break;
        default: me->Say("Golden coins? Bah! I want the heavy tokens!", LANG_UNIVERSAL); break;
    }
}

static void FlavorCiel(Creature* me)
{
    uint32 choice = urand(1, 12);
    switch (choice)
    {
        case 1:  me->Say("Horses? Cats? Dragons? I have them all! Well, mostly horses.", LANG_UNIVERSAL); break;
        case 2:
            me->Say("Do you think this mount makes me look fast?", LANG_UNIVERSAL);
            me->TextEmote("Ciel strikes a pose.");
            break;
        case 3:  me->Say("Walking is so last season. You need hooves! Or wings!", LANG_UNIVERSAL); break;
        case 4:  me->Say("I found a stray gryphon once. It bit me. Stick to the trained ones.", LANG_UNIVERSAL); break;
        case 5:  me->Say("Life in the fast lane! That's where you belong.", LANG_UNIVERSAL); break;
        case 6:  me->Say("Why walk when you can ride in style?", LANG_UNIVERSAL); break;
        case 7:  me->Say("I have mounts that eat gold and poop rainbows. Okay, maybe just eat gold.", LANG_UNIVERSAL); break;
        case 8:  me->Say("Four legs are better than two. It's basic math.", LANG_UNIVERSAL); break;
        case 9:  me->Say("Need a lift? Or just something to show off in Dalaran?", LANG_UNIVERSAL); break;
        case 10:
            me->Say("Check out the horsepower on this beast!", LANG_UNIVERSAL);
            me->TextEmote("Ciel pats a nearby imaginary mount.");
            break;
        case 11: me->Say("My mounts are 100% organic. Except the mechanical ones. Those spill oil.", LANG_UNIVERSAL); break;
        default: me->Say("Catch a ride before I leave!", LANG_UNIVERSAL); break;
    }
}

static void FlavorRename(Creature* me)
{
    uint32 choice = urand(1, 12);
    switch (choice)
    {
        case 1:
            me->Say("I am NOT a transmogger! I change names, not clothes!", LANG_UNIVERSAL);
            me->TextEmote("Rename looks annoyed.");
            break;
        case 2:  me->Say("Names are power. Choose wisely. Or just pick something silly, everyone else does.", LANG_UNIVERSAL); break;
        case 3:  me->Say("You want to rename your demon? Does it really care? FINE.", LANG_UNIVERSAL); break;
        case 4:  me->Say("Don't ask me about my outfit. Just tell me the name.", LANG_UNIVERSAL); break;
        case 5:  me->Say("Please don't pick 'Legolas' again. I beg you.", LANG_UNIVERSAL); break;
        case 6:  me->Say("A rose by any other name... would cost you gold to rename.", LANG_UNIVERSAL); break;
        case 7:  me->Say("Identity crisis? I can help with that.", LANG_UNIVERSAL); break;
        case 8:
            me->Say("You want to call your pet WHAT? ...Okay, no judgment. Maybe a little.", LANG_UNIVERSAL);
            me->TextEmote("Rename raises an eyebrow.");
            break;
        case 9:  me->Say("Paperwork, paperwork. Fill out form 3-B for a name change.", LANG_UNIVERSAL); break;
        case 10: me->Say("New name, new life. Same smell though.", LANG_UNIVERSAL); break;
        case 11: me->Say("I specialize in onomastic reconstruction. That means I rename stuff.", LANG_UNIVERSAL); break;
        default: me->Say("Make it catchy! Something that screams 'Victory'!", LANG_UNIVERSAL); break;
    }
}

static void FlavorWarpweaver(Creature* me)
{
    uint32 choice = urand(1, 12);
    switch (choice)
    {
        case 1:  me->Say("The void calls... do you have the funds to answer?", LANG_UNIVERSAL); break;
        case 2:  me->Say("Reality is fragile. Fashion... is eternal.", LANG_UNIVERSAL); break;
        case 3:  me->Say("I can shape the threads of your existence. For a price.", LANG_UNIVERSAL); break;
        case 4:  me->Say("Everything you see is an illusion. Except my fees. Those are real.", LANG_UNIVERSAL); break;
        case 5:
            me->Say("That helm... with those shoulders? A tragedy.", LANG_UNIVERSAL);
            me->TextEmote("Warpweaver shudders.");
            break;
        case 6:  me->Say("Let us tailor the fabric of the universe to suit your style.", LANG_UNIVERSAL); break;
        case 7:  me->Say("You wish to change your appearance? A wise choice.", LANG_UNIVERSAL); break;
        case 8:  me->Say("The Ethereals know style. You... need assistance.", LANG_UNIVERSAL); break;
        case 9:  me->Say("Purple is the new black. Or is it void-black?", LANG_UNIVERSAL); break;
        case 10: me->Say("Your soul looks... heavy. Maybe lighter fabrics?", LANG_UNIVERSAL); break;
        case 11: me->Say("I weave the nether. And cotton. Mostly nether.", LANG_UNIVERSAL); break;
        default: me->Say("Do not look into the void... unless it matches your boots.", LANG_UNIVERSAL); break;
    }
}

// ============================================================================
// Public API — called from custom_commands.cpp after SummonCreature()
// ============================================================================

void DoSummonFlavorText(Creature* creature, WorldObject* summoner)
{
    if (!creature || !IsFlavorEnabled())
        return;

    std::string name = summoner ? summoner->GetName() : "adventurer";

    switch (creature->GetEntry())
    {
        case 290011: FlavorLing(creature, name);    break;  // Ling (Reagent Bank)
        case 299902: FlavorAshari(creature, name);  break;  // Ashari (Vendor)
        case 299901: FlavorSqueak(creature);        break;  // Lord Squeak (Emblems)
        case 299903: FlavorCiel(creature);          break;  // Ciel (Mounts)
        case 200002: FlavorRename(creature);        break;  // Rename (Pet Renamer)
        case 190011: FlavorWarpweaver(creature);    break;  // Warpweaver (Transmog)
        default: break;
    }
}
