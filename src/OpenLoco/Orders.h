#pragma once
#include "Types.hpp"

namespace OpenLoco::Map
{
    struct map_pos3;
}
namespace OpenLoco::Vehicle
{

#pragma pack(push, 1)
    enum class OrderType
    {
        End,
        StopAt,
        RouteThrough,
        RouteWaypoint,
        UnloadAll,
        WaitFor
    };

    struct Order
    {
        uint8_t _type;
        OrderType getType() const { return OrderType(_type & 0x7); }
        template<typename T>
        constexpr bool is() const { return getType() == T::TYPE; }
        template<typename T>
        T* as() { return is<T>() ? reinterpret_cast<T*>(this) : nullptr; }
        template<typename T>
        const T* as() const { return is<T>() ? reinterpret_cast<const T*>(this) : nullptr; }
    };

    struct OrderEnd : Order
    {
        static constexpr OrderType TYPE = OrderType::End;
    };
    struct OrderStopAt : Order
    {
        static constexpr OrderType TYPE = OrderType::StopAt;
        uint8_t _1;

        station_id_t getStation() const
        {
            return ((_type & 0xC) << 2) | _1;
        }
    };
    struct OrderRouteThrough : Order
    {
        static constexpr OrderType TYPE = OrderType::RouteThrough;
        uint8_t _1;

        station_id_t getStation() const
        {
            return ((_type & 0xC) << 2) | _1;
        }
    };
    struct OrderRouteWaypoint : Order
    {
        static constexpr OrderType TYPE = OrderType::RouteWaypoint;
        uint8_t _1;
        uint8_t _2;
        uint8_t _3;
        uint8_t _4;
        uint8_t _5;

        Map::map_pos3& getWaypoint() const;
    };
    struct OrderUnloadAll : Order
    {
        static constexpr OrderType TYPE = OrderType::UnloadAll;
        uint8_t getCargo() const { return _type >> 3; }
    };
    struct OrderWaitFor : Order
    {
        static constexpr OrderType TYPE = OrderType::WaitFor;
        uint8_t getCargo() const { return _type >> 3; }
    };

#pragma pack(pop)
}
