/*
 * Player Login Grants — Profession unlocks, utility spell grants, racial adjustments,
 *                        universal equipment proficiencies
 * Config: CustomRamvaris.LoginSpellGrants.Enable
 *         CustomRamvaris.AllClassEquip.Enable
 *
 * On login, grants custom utility spells and unlocks profession specializations
 * to all matching characters. Also fixes Draenei racials and grants cross-class
 * abilities like Pick Pocket and Diplomacy to everyone.
 *
 * AllClassEquip: Grants ALL weapon + armor proficiencies to every class so that
 * any character can equip any weapon type, any armor type (including plate + shield),
 * and any class set item. Combined with the AllowableClass DB removal, this lets
 * a Warlock wear Death Knight tier, a Mage dual-wield axes, etc.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"

class custom_player_login_grants : public PlayerScript
{
public:
    custom_player_login_grants() : PlayerScript("custom_player_login_grants") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true))
            return;
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.LoginSpellGrants.Enable", false))
            return;
        if (!player)
            return;

        uint8 playerClass = player->getClass();
        uint8 playerRace  = player->getRace();

        GrantProfessionSpecializations(player);
        GrantCustomSpells(player, playerClass);
        GrantRacialAdjustments(player, playerClass, playerRace);
        CleanupRestrictedSpells(player, playerClass);

        // Universal equipment proficiencies (separate config)
        if (sConfigMgr->GetOption<bool>("CustomRamvaris.AllClassEquip.Enable", false))
            GrantAllProficiencies(player);
    }

private:
    // ========================================================================
    // Profession dual/triple specialization unlock
    // ========================================================================
    static void GrantProfessionSpecializations(Player* player)
    {
        // Engineering (Goblin + Gnomish)
        if (player->HasSpell(20222) || player->HasSpell(20219))
        {
            LearnIfMissing(player, 20222); // Goblin Engineering
            LearnIfMissing(player, 20219); // Gnomish Engineering
        }

        // Alchemy (Potion + Elixir + Transmutation Master)
        if (player->HasSpell(28672) || player->HasSpell(28675) || player->HasSpell(28677))
        {
            LearnIfMissing(player, 28672); // Potion Master
            LearnIfMissing(player, 28675); // Elixir Master
            LearnIfMissing(player, 28677); // Transmutation Master
        }

        // Leatherworking (Dragonscale + Elemental + Tribal)
        if (player->HasSpell(10656) || player->HasSpell(10658) || player->HasSpell(10660))
        {
            LearnIfMissing(player, 10656); // Dragonscale
            LearnIfMissing(player, 10658); // Elemental
            LearnIfMissing(player, 10660); // Tribal
        }

        // Tailoring (Spellfire + Shadoweave + Mooncloth)
        if (player->HasSpell(26797) || player->HasSpell(26801) || player->HasSpell(26798))
        {
            LearnIfMissing(player, 26797); // Spellfire
            LearnIfMissing(player, 26801); // Shadoweave
            LearnIfMissing(player, 26798); // Mooncloth
        }

        // Blacksmithing Tier 1 (Armorsmith + Weaponsmith)
        if (player->HasSpell(9788) || player->HasSpell(9787))
        {
            LearnIfMissing(player, 9788); // Armorsmith
            LearnIfMissing(player, 9787); // Weaponsmith
        }

        // Blacksmithing Tier 2 (Weapon Masteries)
        if (player->HasSpell(17041) || player->HasSpell(17040) || player->HasSpell(17039))
        {
            LearnIfMissing(player, 17041); // Master Swordsmith
            LearnIfMissing(player, 17040); // Master Hammersmith
            LearnIfMissing(player, 17039); // Master Axesmith
        }
    }

    // ========================================================================
    // Custom spell grants (requires custom DBC entries)
    // Unlearns 81002 (Water Form), 81003 (Uber Cheetah), 81008 (Levitation)
    // — replaced by lottery enchant Movespeed/Flying system.
    // ========================================================================
    static void GrantCustomSpells(Player* player, uint8 playerClass)
    {
        // Unlearn spells replaced by lottery enchant speed/fly system
        RemoveIfKnown(player, 81002); // Water Form — swim speed now from enchants
        RemoveIfKnown(player, 81003); // Aspect of Uber Cheetah — run speed now from enchants
        RemoveIfKnown(player, 81008); // Masterful Levitation — flying now from enchants

        // 81005 — not for Rogues
        if (playerClass != CLASS_ROGUE)
            LearnIfMissing(player, 81005);

        LearnIfMissing(player, 81007); // Custom utility spell

        // Diplomacy (Human racial reputation bonus) — for everyone
        LearnIfMissing(player, 20599);

        // Pick Pocket (Rogue ability) — for everyone
        LearnIfMissing(player, 921);
    }

    // ========================================================================
    // Racial adjustments (Draenei fixes, BE Warrior, Human Stoneform,
    // character-specific racial overrides)
    // ========================================================================
    static void GrantRacialAdjustments(Player* player, uint8 playerClass, uint8 playerRace)
    {
        // --- Draenei ---
        if (playerRace == RACE_DRAENEI)
        {
            // Heroic Presence (correct version)
            LearnIfMissing(player, 6562);
            RemoveIfKnown(player, 28878); // Remove wrong Heroic Presence

            // Gift of the Naaru (correct version for custom ARAC)
            LearnIfMissing(player, 59541);
            LearnIfMissing(player, 59542);

            // Remove class-specific Gift of the Naaru variants (we use universal versions)
            RemoveIfKnown(player, 28880); // Warrior
            RemoveIfKnown(player, 59543); // Paladin
            RemoveIfKnown(player, 59544); // Hunter
            RemoveIfKnown(player, 59545); // Priest
            RemoveIfKnown(player, 59548); // Death Knight
            RemoveIfKnown(player, 59547); // Shaman

            // Gemcutting (Draenei racial)
            LearnIfMissing(player, 28875);
        }

        // --- Blood Elf Warrior ---
        if (playerRace == RACE_BLOODELF && playerClass == CLASS_WARRIOR)
            LearnIfMissing(player, 81011); // Custom Arcane Torrent for Warriors

        // --- Human Stoneform (Holy Cleanse) ---
        if (playerRace == RACE_HUMAN)
            LearnIfMissing(player, 81013); // Custom Stoneform (handled by spell_human_stoneform.cpp)

        // --- Character-specific racial overrides ---
        // These run AFTER the engine's learnSkillRewardedSpells re-teaches racials
        // from SkillLineAbility DBC on every login. The only way to persistently
        // remove a DBC-linked racial is to strip it here, post-login.
        ApplyCharacterRacialOverrides(player);
    }

    // ========================================================================
    // Character-specific racial overrides
    // Some characters have lore-specific racial swaps (e.g. a "half-human"
    // Night Elf who trades Shadowmeld for Holy Cleanse).
    // ========================================================================
    static void ApplyCharacterRacialOverrides(Player* player)
    {
        uint32 guid = player->GetGUID().GetCounter();

        switch (guid)
        {
            case 17: // Luna — Night Elf Warrior, "half-human" lore
                RemoveIfKnown(player, 58984);  // Remove Shadowmeld (re-taught by SkillLineAbility DBC every login)
                LearnIfMissing(player, 81013); // Grant Holy Cleanse (Human Stoneform)
                break;
            default:
                break;
        }
    }

    // ========================================================================
    // Remove spells that don't belong to certain classes
    // ========================================================================
    static void CleanupRestrictedSpells(Player* player, uint8 playerClass)
    {
        // Remove 81010 from everyone (deprecated/replaced)
        RemoveIfKnown(player, 81010);

        // Ghost Wolf — Shaman only
        if (playerClass != CLASS_SHAMAN)
            RemoveIfKnown(player, 2645);

        // Aspect of the Cheetah — Hunter only
        if (playerClass != CLASS_HUNTER)
            RemoveIfKnown(player, 5118);

        // Aquatic Form — Druid only
        if (playerClass != CLASS_DRUID)
            RemoveIfKnown(player, 1066);

        // Pick Lock — Rogue only
        if (playerClass != CLASS_ROGUE)
            RemoveIfKnown(player, 1804);

        // 81005 — Rogues shouldn't have it
        if (playerClass == CLASS_ROGUE)
            RemoveIfKnown(player, 81005);

        // 81001 — remove from everyone (deprecated/replaced)
        RemoveIfKnown(player, 81001);
    }

    // ========================================================================
    // Universal weapon + armor proficiencies
    // Config: CustomRamvaris.AllClassEquip.Enable
    // Grants every weapon and armor proficiency spell so any class can
    // equip any item type. Combined with AllowableClass=-1 in item_template,
    // this allows full cross-class equipment (Warlock in plate, etc.).
    // ========================================================================
    static void GrantAllProficiencies(Player* player)
    {
        // ---------- Weapon proficiency spells ----------
        static const uint32 weaponSpells[] = {
            196,   // One-Handed Axes
            197,   // Two-Handed Axes
            198,   // One-Handed Maces
            199,   // Two-Handed Maces
            201,   // One-Handed Swords
            202,   // Two-Handed Swords
            227,   // Staves
            200,   // Polearms
            1180,  // Daggers
            15590, // Fist Weapons
            264,   // Bows
            5011,  // Crossbows
            266,   // Guns
            5009,  // Wands
            2567,  // Thrown
        };

        // ---------- Armor proficiency spells ----------
        static const uint32 armorSpells[] = {
            9078,  // Cloth
            9077,  // Leather
            8737,  // Mail
            750,   // Plate Mail
            9116,  // Shield
        };

        for (uint32 spellId : weaponSpells)
            LearnIfMissing(player, spellId);

        for (uint32 spellId : armorSpells)
            LearnIfMissing(player, spellId);
    }

    // ========================================================================
    // Helpers
    // ========================================================================
    static void LearnIfMissing(Player* player, uint32 spellId)
    {
        if (!player->HasSpell(spellId))
            player->learnSpell(spellId);
    }

    static void RemoveIfKnown(Player* player, uint32 spellId)
    {
        if (player->HasSpell(spellId))
            player->removeSpell(spellId, SPEC_MASK_ALL, false);
    }
};

void AddSC_player_login_grants()
{
    new custom_player_login_grants();
}
