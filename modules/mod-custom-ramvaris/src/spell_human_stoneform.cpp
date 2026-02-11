/*
 * Custom Stoneform for Humans (Spell ID 81013)
 * Removes: Poison, Disease, Curse, Magic debuffs
 * Config: CustomRamvaris.HumanStoneform.Enable
 *
 * DBC Setup: Effect_1 = SPELL_EFFECT_DUMMY, Target = TARGET_UNIT_CASTER,
 *            RecoveryTime = 120000 (2 min cooldown)
 *
 * Dead script for cloners — only active if spell 81013 exists in DBC.
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Unit.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Config.h"

class spell_human_stoneform_81013 : public SpellScript
{
    PrepareSpellScript(spell_human_stoneform_81013);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true))
            return;
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.HumanStoneform.Enable", false))
            return;

        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster)
            return;
        if (!target)
            target = caster;

        // Dispel mask: Poison (1<<4) | Disease (1<<3) | Curse (1<<2) | Magic (1<<1)
        uint32 dispelMask = (1 << DISPEL_POISON) | (1 << DISPEL_DISEASE) | (1 << DISPEL_CURSE) | (1 << DISPEL_MAGIC);

        DispelChargesList dispelList;
        // Note: GetDispellableAuraList is called on the unit that HAS the auras (target),
        // and receives the dispeller (caster) as the first argument.
        target->GetDispellableAuraList(caster, dispelMask, dispelList, GetSpellInfo());

        for (auto itr = dispelList.begin(); itr != dispelList.end(); ++itr)
        {
            if (Aura* aura = itr->first)
            {
                // Remove only debuffs/negative auras (players may have dispellable *buffs*
                // like Blessing/HoT magic auras; this spell is meant to cleanse negatives).
                if (AuraApplication const* app = aura->GetApplicationOfTarget(target->GetGUID()))
                {
                    if (app->IsPositive())
                        continue;
                }

                // IMPORTANT: do NOT filter by caster GUID.
                // The old code removed only self-cast auras, which breaks cleansing of
                // diseases/poisons/etc applied by mobs/players.
                aura->Remove();
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_human_stoneform_81013::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_spell_human_stoneform()
{
    RegisterSpellScript(spell_human_stoneform_81013);
}
