#include "./ScenarioObjective.h"
#include "./GameState.h"

namespace OpenLoco::Scenario
{
    Objective& getObjective()
    {
        return getGameState().scenarioObjective;
    }

    Objective2& getObjective2()
    {
        return getGameState().scenarioObjective2;
    }
}
