#pragma once
#include "../Limits.h"
#include <cstdint>

namespace OpenLoco::Vehicles
{
#pragma pack(push, 1)
    struct RoutingHandle
    {
        uint16_t _data;
        constexpr RoutingHandle(const uint16_t vehicleRef, const uint8_t index)
            : _data((vehicleRef * Limits::kMaxRoutingsPerVehicle) | index)
        {
        }

        constexpr uint16_t getVehicleRef() const { return _data / Limits::kMaxRoutingsPerVehicle; }
        constexpr uint8_t getIndex() const { return _data % Limits::kMaxRoutingsPerVehicle; }
        constexpr void setIndex(uint8_t newIndex)
        {
            _data &= ~(Limits::kMaxRoutingsPerVehicle - 1);
            _data |= newIndex & (Limits::kMaxRoutingsPerVehicle - 1);
        }

        bool operator==(const RoutingHandle other) const { return _data == other._data; }
        bool operator!=(const RoutingHandle other) const { return !(*this == other); }
    };
    static_assert(sizeof(RoutingHandle) == 2);
#pragma pack(pop)
}
