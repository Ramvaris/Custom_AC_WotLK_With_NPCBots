/*
 * Mend Pet Scale Handler — Doubles pet visual scale on each Mend Pet cast
 * Config: CustomRamvaris.MendPetScale.Enable
 *
 * Every time a hunter casts Mend Pet (spell 136), the pet's visual scale
 * is doubled. Caps at 5x to prevent absurdly large pets.
 * Purely cosmetic fun — no gameplay impact.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Spell.h"
#include "Config.h"

enum MendPetConstants
{
    SPELL_MEND_PET  = 136,
    MAX_PET_SCALE   = 5
};

class custom_mend_pet_scale : public PlayerScript
{
public:
    custom_mend_pet_scale() : PlayerScript("custom_mend_pet_scale") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true))
            return;
        if (!sConfigMgr->GetOption<bool>("CustomRamvaris.MendPetScale.Enable", false))
            return;
        if (!player || !spell)
            return;
        if (spell->GetSpellInfo()->Id != SPELL_MEND_PET)
            return;

        Pet* pet = player->GetPet();
        if (!pet)
            return;

        float currentScale = pet->GetObjectScale();
        float newScale = currentScale * 2.0f;

        if (newScale > static_cast<float>(MAX_PET_SCALE))
            newScale = static_cast<float>(MAX_PET_SCALE);

        pet->SetObjectScale(newScale);
    }
};

void AddSC_player_mend_pet_scale()
{
    new custom_mend_pet_scale();
}
