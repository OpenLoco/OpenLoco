#include "./ScenarioConstruction.h"
#include "./GameState.h"

namespace OpenLoco::Scenario
{
    Construction& getConstruction()
    {
        return getGameState().scenarioConstruction;
    }

    void resetConstruction() {
        std::fill(std::begin(Scenario::getConstruction().signals), std::end(Scenario::getConstruction().signals), 0xFF);
        std::fill(std::begin(Scenario::getConstruction().bridges), std::end(Scenario::getConstruction().bridges), 0xFF);
        std::fill(std::begin(Scenario::getConstruction().trainStations), std::end(Scenario::getConstruction().trainStations), 0xFF);
        std::fill(std::begin(Scenario::getConstruction().trackMods), std::end(Scenario::getConstruction().trackMods), 0xFF);
    }
}
