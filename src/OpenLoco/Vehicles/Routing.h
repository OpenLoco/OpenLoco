#pragma once
#include <cstdint>

namespace OpenLoco::Vehicles
{
    constexpr auto max_num_routing_steps = 64;
#pragma pack(push, 1)
    struct RoutingHandle
    {
        uint16_t _data;
        constexpr RoutingHandle(const uint16_t vehicleRef, const uint8_t index)
            : _data((vehicleRef * max_num_routing_steps) | index)
        {
        }

        constexpr uint16_t getVehicleRef() const { return _data / max_num_routing_steps; }
        constexpr uint8_t getIndex() const { return _data % max_num_routing_steps; }
        constexpr void setIndex(uint8_t newIndex)
        {
            _data &= ~(max_num_routing_steps - 1);
            _data |= newIndex & (max_num_routing_steps - 1);
        }

        bool operator==(const RoutingHandle other) { return _data == other._data; }
        bool operator!=(const RoutingHandle other) { return !(*this == other); }
    };
    static_assert(sizeof(RoutingHandle) == 2);
#pragma pack(pop)
}
