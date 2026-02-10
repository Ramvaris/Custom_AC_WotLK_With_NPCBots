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
        if (!caster)
            return;

        // Dispel mask: Poison (1<<4) | Disease (1<<3) | Curse (1<<2) | Magic (1<<1)
        uint32 dispelMask = (1 << DISPEL_POISON) | (1 << DISPEL_DISEASE) | (1 << DISPEL_CURSE) | (1 << DISPEL_MAGIC);

        DispelChargesList dispelList;
        caster->GetDispellableAuraList(caster, dispelMask, dispelList, GetSpellInfo());

        for (auto itr = dispelList.begin(); itr != dispelList.end(); ++itr)
        {
            if (Aura* aura = itr->first)
                caster->RemoveAurasDueToSpell(aura->GetId(), caster->GetGUID());
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
