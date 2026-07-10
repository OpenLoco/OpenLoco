#include "GameState.h"

namespace OpenLoco
{
    static GameState _gameState;

    GameState& getGameState()
    {
        return _gameState;
    }
}
