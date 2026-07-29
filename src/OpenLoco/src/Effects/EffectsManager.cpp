#include "Effects/EffectsManager.h"
#include "GameState.h"
#include "GameStateFlags.h"

namespace OpenLoco::EffectsManager
{

    // 0x004402F4
    void tick()
    {
        if ((getGameState().flags & GameStateFlags::tileManagerLoaded) != GameStateFlags::none)
        {
            for (auto* misc : EffectsList())
            {
                misc->tick();
            }
        }
    }

}
