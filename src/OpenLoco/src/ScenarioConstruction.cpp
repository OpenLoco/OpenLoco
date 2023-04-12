#include "./GameState.h"
#include "./ScenarioConstruction.h"

namespace OpenLoco::Scenario
{
    Construction& getConstruction() {
        return getGameState().scenarioConstruction;
    }
}
