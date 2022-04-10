#include "./Misc.h"
#include "../GameState.h"

namespace OpenLoco
{
    // 0x00525E28
    uint32_t& gameStateFlags()
    {
        return getGameState().flags;
    }
}
