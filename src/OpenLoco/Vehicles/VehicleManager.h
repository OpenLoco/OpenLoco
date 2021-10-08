#pragma once
#include "Routing.h"
#include <optional>

namespace OpenLoco
{
    struct Company;
}

namespace OpenLoco::VehicleManager
{
    void determineAvailableVehicles(Company& company);
}

namespace OpenLoco::Vehicles::RoutingManager
{
    std::optional<RoutingHandle> getAndAllocateFreeRoutingHandle();
    void freeRoutingHandle(const RoutingHandle routing);
    uint16_t getRouting(const RoutingHandle routing);
    void freeRouting(const RoutingHandle routing);
    bool isEmptyRoutingSlotAvailable();
}
