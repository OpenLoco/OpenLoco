#pragma once
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    struct VehicleTail : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::tail;
        VehicleSound sound;
        uint16_t trainDanglingTimeout; // counts up when no cars on train

        bool update();
    };
    static_assert(sizeof(VehicleTail) <= sizeof(Entity));
}
