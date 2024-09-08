#include "./LastGameOptionManager.h"
#include "./GameState.h"

namespace OpenLoco::LastGameOptionManager
{
    void setLastRailRoad(uint8_t last)
    {
        getGameState().lastRailroadOption = last;
    }

    void setLastRoad(uint8_t last)
    {
        getGameState().lastRoadOption = last;
    }

    void setLastAirport(uint8_t last)
    {
        getGameState().lastAirport = last;
    }

    void setLastShipPort(uint8_t last)
    {
        getGameState().lastShipPort = last;
    }

    void setLastVehicleType(VehicleType& last)
    {
        getGameState().lastVehicleType = last;
    }

    void setLastTree(uint8_t last)
    {
        getGameState().lastTreeOption = last;
    }

    void setLastLand(uint8_t last)
    {
        getGameState().lastLandOption = last;
    }

    void setLastTrackType(uint8_t last)
    {
        getGameState().lastTrackTypeOption = last;
    }

    void setLastIndustry(uint8_t last)
    {
        getGameState().lastIndustryOption = last;
    }

    void setLastBuildingOption(uint8_t last)
    {
        getGameState().lastBuildingOption = last;
    }

    void setLastMiscBuildingOption(uint8_t last)
    {
        getGameState().lastMiscBuildingOption = last;
    }

    void setLastWall(uint8_t last)
    {
        getGameState().lastWallOption = last;
    }

    void setLastBuildVehiclesOption(uint8_t last)
    {
        getGameState().lastBuildVehiclesOption = last;
    }
}
