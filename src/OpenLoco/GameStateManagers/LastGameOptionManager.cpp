#include "./LastGameOptionManager.h"
#include "../GameState.h"

namespace OpenLoco::LastGameOptionManager
{
    // 0x00525FAA
    uint8_t getLastRailRoadOption()
    {
        return getGameState().lastRailroadOption;
    }
    void setLastRailRoadOption(uint8_t last)
    {
        getGameState().lastRailroadOption = last;
    }

    // 0x00525FAB
    uint8_t getLastRoadOption()
    {
        return getGameState().lastRoadOption;
    }
    void setLastRoadOption(uint8_t last)
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
    uint8_t getLastTreeOption()
    {
        return getGameState().lastTreeOption;
    }
    void setLastTreeOption(uint8_t last)
    {
        getGameState().lastTreeOption = last;
    }

    // 0x00525FB6
    uint8_t getLastLandOption()
    {
        return getGameState().lastLandOption;
    }
    void setLastLandOption(uint8_t last)
    {
        getGameState().lastLandOption = last;
    }

    // 0x00525FC7
    uint8_t getLastIndustryOption()
    {
        return getGameState().lastIndustryOption;
    }
    void setLastIndustryOption(uint8_t last)
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
    uint8_t getLastWallOption()
    {
        return getGameState().lastWallOption;
    }
    void setLastWallOption(uint8_t last)
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
