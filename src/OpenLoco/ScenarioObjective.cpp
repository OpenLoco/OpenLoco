#include "ScenarioObjective.h"
#include "./GameState.h"

namespace OpenLoco::Scenario
{
    Objective& getObjective()
    {
        return getGameState().scenarioObjective;
    }
}
