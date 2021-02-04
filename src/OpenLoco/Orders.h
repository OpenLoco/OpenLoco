#pragma once
#include "Map/Tile.h"
#include "Types.hpp"
#include <iterator>

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
        uint32_t getOffset() const;
        template<typename T>
        constexpr bool is() const { return getType() == T::TYPE; }
        template<typename T>
        T* as() { return is<T>() ? reinterpret_cast<T*>(this) : nullptr; }
        template<typename T>
        const T* as() const { return is<T>() ? reinterpret_cast<const T*>(this) : nullptr; }
    };

    static_assert(sizeof(Order) == 1, "Size of order must be 1 for pointer arithmatic to work in OrderTableView");

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

        Map::map_pos3 getWaypoint() const;
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

    struct OrderTableView
    {
    private:
        struct Iterator
        {
        private:
            Order* _orders;

        public:
            Iterator(Order* orders)
                : _orders(orders)
            {
            }

            Iterator& operator++();

            Iterator operator++(int)
            {
                Iterator res = *this;
                ++(*this);
                return res;
            }

            bool operator==(Iterator other) const
            {
                return _orders->getType() == other->getType();
            }

            bool operator!=(Iterator other) const
            {
                return !(*this == other);
            }

            Order& operator*()
            {
                return *_orders;
            }

            const Order& operator*() const
            {
                return *_orders;
            }

            Order* operator->()
            {
                return _orders;
            }

            const Order* operator->() const
            {
                return _orders;
            }

            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = Order;
            using pointer = const Order*;
            using reference = const Order&;
            using iterator_category = std::forward_iterator_tag;
        };

        uint32_t _beginOffset;

    public:
        OrderTableView(uint32_t offset)
            : _beginOffset(offset)
        {
        }

        OrderTableView::Iterator begin();
        OrderTableView::Iterator end();
    };
}
