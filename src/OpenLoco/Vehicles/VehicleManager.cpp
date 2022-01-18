#include "VehicleManager.h"
#include "../Company.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::VehicleManager
{
    // 0x004C3A0C
    void determineAvailableVehicles(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x004C3A0C, regs);
    }
}

namespace OpenLoco::Vehicles::RoutingManager
{
    constexpr uint16_t kAllocatedButFreeRoutingStation = -2; // Indicates that this array is allocated to a vehicle but no station has been set.
    constexpr uint16_t kRoutingNull = -1;                        // Indicates that this array is allocated to a vehicle but no station has been set.

    static auto& routings() { return getGameState().routings; }

    static std::optional<uint16_t> findFreeRoutingVehicleRef()
    {
        const auto& routingArr = routings();
        const auto res = std::find_if(std::begin(routingArr), std::end(routingArr), [](const auto& route) { return route[0] == kRoutingNull; });
        if (res == std::end(routingArr))
        {
            return std::nullopt;
        }
        return std::distance(std::begin(routingArr), res);
    }

    bool isEmptyRoutingSlotAvailable()
    {
        return findFreeRoutingVehicleRef().has_value();
    }

    // 0x004B1E00
    std::optional<RoutingHandle> getAndAllocateFreeRoutingHandle()
    {
        auto vehicleRef = findFreeRoutingVehicleRef();
        if (vehicleRef.has_value())
        {
            auto& vehRoutingArr = routings()[*vehicleRef];
            std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kAllocatedButFreeRoutingStation);
            return { RoutingHandle(*vehicleRef, 0) };
        }
        return std::nullopt;
    }

    uint16_t getRouting(const RoutingHandle routing)
    {
        return routings()[routing.getVehicleRef()][routing.getIndex()];
    }

    void freeRouting(const RoutingHandle routing)
    {
        routings()[routing.getVehicleRef()][routing.getIndex()] = kAllocatedButFreeRoutingStation;
    }

    // 0x004B1E77
    void freeRoutingHandle(const RoutingHandle routing)
    {
        auto& vehRoutingArr = routings()[routing.getVehicleRef()];
        std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kRoutingNull);
    }
}
