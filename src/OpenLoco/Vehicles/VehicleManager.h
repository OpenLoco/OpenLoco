#pragma once

namespace OpenLoco
{
    struct Company;
}

namespace OpenLoco::VehicleManager
{
    void determineAvailableVehicles(Company& company);
}
