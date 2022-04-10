#include "./GameState.h"
#include "./LastWindowState.h"

namespace OpenLoco
{
    LastWindowState& getLastWindowState()
    {
        return getGameState().lastWindowState;
    }
}
