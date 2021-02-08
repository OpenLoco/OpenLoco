#pragma once
#include "../Types.hpp"

namespace OpenLoco::Vehicles
{
    struct vehicle_base;
}

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintVehicleEntity(PaintSession& session, Vehicles::vehicle_base* base);
}
