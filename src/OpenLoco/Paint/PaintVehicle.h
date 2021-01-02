#pragma once
#include "../Types.hpp"

namespace OpenLoco
{
    struct vehicle_base;
}

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintVehicleEntity(PaintSession& session, vehicle_base* base);
}
