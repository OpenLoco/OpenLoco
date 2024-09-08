#pragma once

#include "./Types.hpp"
#include <cstdint>

namespace OpenLoco::LastGameOptionManager
{
    void setLastRailRoad(uint8_t last);
    void setLastRoad(uint8_t last);
    void setLastAirport(uint8_t last);
    void setLastShipPort(uint8_t last);
    void setLastVehicleType(VehicleType& last);
    void setLastTree(uint8_t last);
    void setLastLand(uint8_t last);
    void setLastTrackType(uint8_t last);
    void setLastIndustry(uint8_t last);
    void setLastBuildingOption(uint8_t last);
    void setLastMiscBuildingOption(uint8_t last);
    void setLastWall(uint8_t last);
    void setLastBuildVehiclesOption(uint8_t last);
}
