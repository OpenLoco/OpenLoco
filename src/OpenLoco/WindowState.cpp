#include "./GameState.h"
#include "./LastWindowState.h"

namespace OpenLoco
{
    // 0x00525FAF
    VehicleType getLastVehicleType()
    {
        return getGameState().lastVehicleType;
    }

    void setLastVehicleType(VehicleType& last)
    {
        getGameState().lastVehicleType = last;
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

    // 0x00525FB6
    uint8_t getLastLandOption()
    {
        return getGameState().lastLandOption;
    }
    void setLastLandOption(uint8_t last)
    {
        getGameState().lastLandOption = last;
    }
}
