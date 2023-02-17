#include "EffectsManager.h"
#include "GameState.h"
#include "GameStateFlags.h"

namespace OpenLoco::EffectsManager
{

    // 0x004402F4
    void update()
    {
        if ((getGameState().flags & GameStateFlags::tileManagerLoaded) != GameStateFlags::none)
        {
            for (auto* misc : EffectsList())
            {
                misc->update();
            }
        }
    }

}
