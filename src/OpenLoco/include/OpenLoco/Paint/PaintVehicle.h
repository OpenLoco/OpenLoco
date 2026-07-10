#pragma once
#include "Types.hpp"

namespace OpenLoco::Vehicles
{
    struct VehicleBase;
}

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintVehicleEntity(PaintSession& session, Vehicles::VehicleBase* base);
}
