#include "Ui/LastMapWindowAttributes.h"
#include "GameState.h"

namespace OpenLoco::Ui
{
    LastMapWindowAttributes& getLastMapWindowAttributes()
    {
        return getGameState().lastMapWindowAttributes;
    }
}
