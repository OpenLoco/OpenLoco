#include "./ScenarioObjective.h"
#include "./GameState.h"

namespace OpenLoco::Scenario
{
    Objective& getObjective()
    {
        return getGameState().scenarioObjective;
    }

    ObjectiveProgress& getObjectiveProgress()
    {
        return getGameState().scenarioObjectiveProgress;
    }
}
