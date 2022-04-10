#include "./GameState.h"
#include "./LastWindowState.h"

namespace OpenLoco
{
    LastWindowState& getLastWindowState()
    {
        return getGameState().lastWindowState;
    }
}

namespace OpenLoco::LastWindowOption
{
    // uint8_t getLastIndustryOption()
    //{
    //     return getGameState().lastIndustryOption;
    // }
    // void setLastIndustryOption(uint8_t last)
    //{
    //     getGameState().lastIndustryOption = last;
    // }

    // uint8_t getLastBuildingOption()
    //{
    //     return getGameState().lastBuildingOption;
    // }
    // void setLastBuildingOption(uint8_t last)
    //{
    //     getGameState().lastBuildingOption = last;
    // }

    // uint8_t getLastMiscBuildingOption()
    //{
    //     return getGameState().lastMiscBuildingOption;
    // }
    // void setLastMiscBuildingOption(uint8_t last)
    //{
    //     getGameState().lastMiscBuildingOption = last;
    // }

    // uint8_t getLastWallOption()
    //{
    //     return getGameState().lastWallOption;
    // }
    // void setLastWallOption(uint8_t last)
    //{
    //     getGameState().lastWallOption = last;
    // }

    // uint8_t getLastBuildVehiclesOption()
    //{
    //     return getGameState().lastBuildVehiclesOption;
    // }
    // void setLastBuildVehiclesOption(uint8_t last)
    //{
    //     getGameState().lastBuildVehiclesOption = last;
    // }
}
