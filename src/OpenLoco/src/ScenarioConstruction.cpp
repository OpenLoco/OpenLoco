#include "./ScenarioConstruction.h"
#include "./GameState.h"

namespace OpenLoco::Scenario
{
    Construction& getConstruction()
    {
        return getGameState().scenarioConstruction;
    }

    // 0x00475988
    void resetRoadObjects()
    {
        auto& construction = getGameState().scenarioConstruction;
        for (auto i = 0U; i < 8; i++)
        {
            construction.var_17A[i] = 0xFF;
            construction.roadStations[i] = 0xFF;
            construction.roadMods[i] = 0xFF;
        }
    }

    void resetTrackObjects()
    {
        auto& construction = getConstruction();
        std::fill(std::begin(construction.signals), std::end(construction.signals), 0xFF);
        std::fill(std::begin(construction.bridges), std::end(construction.bridges), 0xFF);
        std::fill(std::begin(construction.trainStations), std::end(construction.trainStations), 0xFF);
        std::fill(std::begin(construction.trackMods), std::end(construction.trackMods), 0xFF);
    }
}
