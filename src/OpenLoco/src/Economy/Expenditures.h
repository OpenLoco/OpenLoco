#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
    enum ExpenditureType : uint8_t
    {
        TrainIncome,
        TrainRunningCosts,
        BusIncome,
        BusRunningCosts,
        TruckIncome,
        TruckRunningCosts,
        TramIncome,
        TramRunningCosts,
        AircraftIncome,
        AircraftRunningCosts,
        ShipIncome,
        ShipRunningCosts,
        Construction,
        VehiclePurchases,
        VehicleDisposals,
        LoanInterest,
        Miscellaneous,
        Count
    };
}
