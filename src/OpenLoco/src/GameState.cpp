#include "GameState.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    loco_global<GameState, 0x00525E18> _gameState;

    GameState& getGameState()
    {
        return *_gameState;
    }
}
