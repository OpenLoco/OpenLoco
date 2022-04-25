#pragma once

#include "../Types.hpp"
#include <cstdint>

namespace OpenLoco::LastGameOptionManager
{
    constexpr uint8_t kNoLastWallOption = 0xFF;
    constexpr uint8_t kNoLastTreeOption = 0xFF;
    constexpr uint8_t kNoLastRoadOption = 0xFF;
    constexpr uint8_t kNoLastLandOption = 0xFF;

    // 0x00525FAA
    uint8_t getLastRailRoadOption();
    void setLastRailRoadOption(uint8_t last);

    // 0x00525FAB
    uint8_t getLastRoadOption();
    void setLastRoadOption(uint8_t last);

    // 0x00525FAC
    uint8_t getLastAirport();
    void setLastAirport(uint8_t last);

    // 0x00525FAD
    uint8_t getLastShipPort();
    void setLastShipPort(uint8_t last);

    // 0x00525FAF
    VehicleType getLastVehicleType();
    void setLastVehicleType(VehicleType& last);

    // 0x00525FB1
    uint8_t getLastTreeOption();
    void setLastTreeOption(uint8_t last);

    // 0x00525FB6
    uint8_t getLastLandOption();
    void setLastLandOption(uint8_t last);

    // 0x00525FC7
    uint8_t getLastIndustryOption();
    void setLastIndustryOption(uint8_t last);

    // 0x00525FC8
    uint8_t getLastBuildingOption();
    void setLastBuildingOption(uint8_t last);

    // 0x00525FC9
    uint8_t getLastMiscBuildingOption();
    void setLastMiscBuildingOption(uint8_t last);

    // 0x00525FCA
    uint8_t getLastWallOption();
    void setLastWallOption(uint8_t last);

    // 0x0052622C
    uint8_t getLastBuildVehiclesOption();
    void setLastBuildVehiclesOption(uint8_t last);

    // Ui::Size lastMapWindowSize;   // 0x000470 (0x00526288)
    // uint16_t lastMapWindowVar88A; // 0x000474 (0x0052628C)
    // uint16_t lastMapWindowVar88C; // 0x000476 (0x0052628E)
}
