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
        void setType(OrderType type)
        {
            _type &= ~0x7;
            _type |= static_cast<uint8_t>(type);
        }
        uint32_t getOffset() const;
        std::shared_ptr<Order> clone() const;
        uint64_t getRaw() const;

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

    struct OrderStation : Order
    {
        uint8_t _1;

        station_id_t getStation() const
        {
            return ((_type & 0xC0) << 2) | _1;
        }
        void setStation(station_id_t station)
        {
            _type &= ~(0xC0);
            _type |= (station >> 2) & 0xC0;
            _1 = station & 0xFF;
        }
        void setFormatArguments(FormatArguments& args) const;
    };

    struct OrderStopAt : OrderStation
    {
        static constexpr OrderType TYPE = OrderType::StopAt;
    };

    struct OrderRouteThrough : OrderStation
    {
        static constexpr OrderType TYPE = OrderType::RouteThrough;
    };

    struct OrderRouteWaypoint : Order
    {
        static constexpr OrderType TYPE = OrderType::RouteWaypoint;
        uint8_t _1;
        uint8_t _2;
        uint8_t _3;
        uint8_t _4;
        uint8_t _5;

        void setWaypoint(const Map::TilePos& pos, const uint8_t baseZ);
        void setDirection(const uint8_t direction);
        void setTrackId(const uint8_t trackId);
        Map::map_pos3 getWaypoint() const;
        uint8_t getDirection() const;
        uint8_t getTrackId() const;
    };

    struct OrderCargo : Order
    {
        uint8_t getCargo() const { return _type >> 3; }
        void setCargo(uint8_t cargo)
        {
            _type &= ~0xF8;
            _type |= (cargo & 0x1F) << 3;
        }
        void setFormatArguments(FormatArguments& args) const;
    };

    struct OrderUnloadAll : OrderCargo
    {
        static constexpr OrderType TYPE = OrderType::UnloadAll;
    };
    struct OrderWaitFor : OrderCargo
    {
        static constexpr OrderType TYPE = OrderType::WaitFor;
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
