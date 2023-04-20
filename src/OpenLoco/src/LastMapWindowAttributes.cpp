#include "LastMapWindowAttributes.h"
#include "GameState.h"

namespace OpenLoco::LastMapWindow
{
    LastMapWindowAttributes& getLastMapWindowAttributes() {
        return getGameState().lastMapWindowAttributes;
    }
}
