#pragma once

#include "./Types.hpp"
#include <cstdint>

namespace OpenLoco::LastGameOptionManager
{
    constexpr uint8_t kNoLastOption = 0xFF;

    // 0x00525FAA
    uint8_t getLastRailRoad();
    void setLastRailRoad(uint8_t last);

    // 0x00525FAB
    uint8_t getLastRoad();
    void setLastRoad(uint8_t last);

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
    uint8_t getLastTree();
    void setLastTree(uint8_t last);

    // 0x00525FB6
    uint8_t getLastLand();
    void setLastLand(uint8_t last);

    // 0x00525FC5
    uint8_t getLastTrackType();
    void setLastTrackType(uint8_t last);

    // 0x00525FC7
    uint8_t getLastIndustry();
    void setLastIndustry(uint8_t last);

    // 0x00525FC8
    uint8_t getLastBuildingOption();
    void setLastBuildingOption(uint8_t last);

    // 0x00525FC9
    uint8_t getLastMiscBuildingOption();
    void setLastMiscBuildingOption(uint8_t last);

    // 0x00525FCA
    uint8_t getLastWall();
    void setLastWall(uint8_t last);

    // 0x0052622C
    uint8_t getLastBuildVehiclesOption();
    void setLastBuildVehiclesOption(uint8_t last);
}
