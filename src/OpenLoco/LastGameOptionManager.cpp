#include "./LastGameOptionManager.h"
#include "./GameState.h"

namespace OpenLoco::LastGameOptionManager
{
    // 0x00525FAA
    uint8_t getLastRailRoad()
    {
        return getGameState().lastRailroadOption;
    }
    void setLastRailRoad(uint8_t last)
    {
        getGameState().lastRailroadOption = last;
    }

    // 0x00525FAB
    uint8_t getLastRoad()
    {
        return getGameState().lastRoadOption;
    }
    void setLastRoad(uint8_t last)
    {
        getGameState().lastRoadOption = last;
    }

    // 0x00525FAC
    uint8_t getLastAirport()
    {
        return getGameState().lastAirport;
    }
    void setLastAirport(uint8_t last)
    {
        getGameState().lastAirport = last;
    }

    // 0x00525FAD
    uint8_t getLastShipPort()
    {
        return getGameState().lastShipPort;
    }
    void setLastShipPort(uint8_t last)
    {
        getGameState().lastShipPort = last;
    }

    // 0x00525FAF
    VehicleType getLastVehicleType()
    {
        return getGameState().lastVehicleType;
    }

    void setLastVehicleType(VehicleType& last)
    {
        getGameState().lastVehicleType = last;
    }

    // 0x00525FB1
    uint8_t getLastTree()
    {
        return getGameState().lastTreeOption;
    }
    void setLastTree(uint8_t last)
    {
        getGameState().lastTreeOption = last;
    }

    // 0x00525FB6
    uint8_t getLastLand()
    {
        return getGameState().lastLandOption;
    }
    void setLastLand(uint8_t last)
    {
        getGameState().lastLandOption = last;
    }

    // 0x00525FC7
    uint8_t getLastIndustry()
    {
        return getGameState().lastIndustryOption;
    }
    void setLastIndustry(uint8_t last)
    {
        getGameState().lastIndustryOption = last;
    }

    // 0x00525FC8
    uint8_t getLastBuildingOption()
    {
        return getGameState().lastBuildingOption;
    }
    void setLastBuildingOption(uint8_t last)
    {
        getGameState().lastBuildingOption = last;
    }

    // 0x00525FC9
    uint8_t getLastMiscBuildingOption()
    {
        return getGameState().lastMiscBuildingOption;
    }
    void setLastMiscBuildingOption(uint8_t last)
    {
        getGameState().lastMiscBuildingOption = last;
    }

    // 0x00525FCA
    uint8_t getLastWall()
    {
        return getGameState().lastWallOption;
    }
    void setLastWall(uint8_t last)
    {
        getGameState().lastWallOption = last;
    }

    // 0x0052622C
    uint8_t getLastBuildVehiclesOption()
    {
        return getGameState().lastBuildVehiclesOption;
    }
    void setLastBuildVehiclesOption(uint8_t last)
    {
        getGameState().lastBuildVehiclesOption = last;
    }

    // Ui::Size lastMapWindowSize;   // 0x000470 (0x00526288)
    // uint16_t lastMapWindowVar88A; // 0x000474 (0x0052628C)
    // uint16_t lastMapWindowVar88C; // 0x000476 (0x0052628E)
}
