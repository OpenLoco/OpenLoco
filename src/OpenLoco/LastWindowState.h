#pragma once

#include "./Types.hpp"
#include <cstdint>

namespace OpenLoco::GameState::LastOption
{
    // 0x00525FAF
    VehicleType getLastVehicleType();
    void setLastVehicleType(VehicleType& last);

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

    // VehicleType lastVehicleType;     // 0x000197 (0x00525FAF)
    // uint8_t lastIndustryOption;      // 0x0001AF (0x00525FC7)
    // uint8_t lastBuildingOption;      // 0x0001B0 (0x00525FC8)
    // uint8_t lastMiscBuildingOption;  // 0x0001B1 (0x00525FC9)
    // uint8_t lastWallOption;          // 0x0001B2 (0x00525FCA)
    // uint8_t lastBuildVehiclesOption; // 0x000414 (0x0052622C)
    // uint32_t lastMapWindowFlags;     // 0x00046C (0x00526284)

    // Ui::Size lastMapWindowSize;   // 0x000470 (0x00526288)
    // uint16_t lastMapWindowVar88A; // 0x000474 (0x0052628C)
    // uint16_t lastMapWindowVar88C; // 0x000476 (0x0052628E)
    // uint8_t lastRailroadOption;   // 0x000192 (0x00525FAA)
    // uint8_t lastRoadOption;       // 0x000193 (0x00525FAB)
    // uint8_t lastAirport;          // 0x000194 (0x00525FAC)
    // uint8_t lastShipPort;         // 0x000195 (0x00525FAD)
    // uint8_t lastTreeOption;       // 0x000199 (0x00525FB1)
    // uint8_t lastLandOption;       // 0x00019E (0x00525FB6)

}
