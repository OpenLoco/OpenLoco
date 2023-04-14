#include "./ScenarioConstruction.h"
#include "./GameState.h"

namespace OpenLoco::Scenario
{
    Construction& getConstruction()
    {
        return getGameState().scenarioConstruction;
    }

    void resetConstruction()
    {
        auto& construction = getConstruction();
        std::fill(std::begin(construction.signals), std::end(construction.signals), 0xFF);
        std::fill(std::begin(construction.bridges), std::end(construction.bridges), 0xFF);
        std::fill(std::begin(construction.trainStations), std::end(construction.trainStations), 0xFF);
        std::fill(std::begin(construction.trackMods), std::end(construction.trackMods), 0xFF);
    }
}
