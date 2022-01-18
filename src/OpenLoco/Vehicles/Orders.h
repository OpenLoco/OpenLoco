#pragma once
#include "../Map/Tile.h"
#include "../Types.hpp"
#include <iterator>
#include <memory>

namespace OpenLoco::Vehicles
{
    enum class OrderType : uint8_t
    {
        End,
        StopAt,
        RouteThrough,
        RouteWaypoint,
        UnloadAll,
        WaitFor
    };

    namespace OrderFlags
    {
        constexpr uint8_t IsRoutable = (1 << 0);
        constexpr uint8_t HasNumber = (1 << 1);
        constexpr uint8_t HasCargo = (1 << 2);
        constexpr uint8_t HasStation = (1 << 3);
    }

#pragma pack(push, 1)
    struct Order
    {
        uint8_t _type = 0; // 0x0

    protected:
        Order() = default;

    public:
        OrderType getType() const { return OrderType(_type & 0x7); }
        void setType(OrderType type)
        {
            _type &= ~0x7;
            _type |= static_cast<uint8_t>(type);
        }
        uint32_t getOffset() const;
        std::shared_ptr<Order> clone() const;
        uint64_t getRaw() const;
        bool hasFlag(const uint8_t flag) const;

        template<typename T>
        constexpr bool is() const { return getType() == T::kType; }

        template<typename T>
        T* as() { return is<T>() ? reinterpret_cast<T*>(this) : nullptr; }
        template<typename T>
        const T* as() const { return is<T>() ? reinterpret_cast<const T*>(this) : nullptr; }
    };
    static_assert(sizeof(Order) == 1, "Size of order must be 1 for pointer arithmatic to work in OrderTableView");

    struct OrderEnd : Order
    {
        static constexpr OrderType kType = OrderType::End;
    };

    struct OrderStation : Order
    {
        uint8_t _data = 0; // 0x1

        StationId getStation() const
        {
            return StationId(((_type & 0xC0) << 2) | _data);
        }
        void setStation(const StationId station)
        {
            _type &= ~(0xC0);
            _type |= (enumValue(station) >> 2) & 0xC0;
            _data = enumValue(station) & 0xFF;
        }
        void setFormatArguments(FormatArguments& args) const;
    };

    struct OrderStopAt : OrderStation
    {
        static constexpr OrderType kType = OrderType::StopAt;
        OrderStopAt(const StationId station)
        {
            setType(kType);
            setStation(station);
        }
    };

    struct OrderRouteThrough : OrderStation
    {
        static constexpr OrderType kType = OrderType::RouteThrough;
        OrderRouteThrough(const StationId station)
        {
            setType(kType);
            setStation(station);
        }
    };

    template<>
    constexpr bool Order::is<OrderStation>() const
    {
        if (is<OrderStopAt>())
        {
            return true;
        }
        return is<OrderRouteThrough>();
    }

    struct OrderRouteWaypoint : Order
    {
        static constexpr OrderType kType = OrderType::RouteWaypoint;
        uint8_t _data[5] = { 0 }; // 0x1 - 0x6

        OrderRouteWaypoint(const Map::TilePos2& pos, const uint8_t baseZ, const uint8_t direction, const uint8_t trackId)
        {
            setType(kType);
            setWaypoint(pos, baseZ);
            setDirection(direction);
            setTrackId(trackId);
        }
        void setWaypoint(const Map::TilePos2& pos, const uint8_t baseZ);
        void setDirection(const uint8_t direction);
        void setTrackId(const uint8_t trackId);
        Map::Pos3 getWaypoint() const;
        uint8_t getDirection() const;
        uint8_t getTrackId() const;
    };

    struct OrderCargo : Order
    {
        uint8_t getCargo() const { return _type >> 3; }
        void setCargo(const uint8_t cargo)
        {
            _type &= ~0xF8;
            _type |= (cargo & 0x1F) << 3;
        }
        void setFormatArguments(FormatArguments& args) const;
    };

    struct OrderUnloadAll : OrderCargo
    {
        static constexpr OrderType kType = OrderType::UnloadAll;
        OrderUnloadAll(const uint8_t cargo)
        {
            setType(kType);
            setCargo(cargo);
        }
    };
    struct OrderWaitFor : OrderCargo
    {
        static constexpr OrderType kType = OrderType::WaitFor;
        OrderWaitFor(const uint8_t cargo)
        {
            setType(kType);
            setCargo(cargo);
        }
    };

#pragma pack(pop)

    struct OrderRingView
    {
    private:
        struct Iterator
        {
        private:
            Order* _beginOrderTable;
            Order* _currentOrder;
            bool _hasLooped = false;

        public:
            Iterator(Order* beginOrderTable, Order* currentOrder)
                : _beginOrderTable(beginOrderTable)
                , _currentOrder(currentOrder)
            {
                // Prevent empty tables looping
                if (_currentOrder->getType() == OrderType::End)
                {
                    _hasLooped = true;
                }
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
                return _currentOrder == other._currentOrder && (_hasLooped || other._hasLooped);
            }

            bool operator!=(Iterator other) const
            {
                return !(*this == other);
            }

            Order& operator*()
            {
                return *_currentOrder;
            }

            const Order& operator*() const
            {
                return *_currentOrder;
            }

            Order* operator->()
            {
                return _currentOrder;
            }

            const Order* operator->() const
            {
                return _currentOrder;
            }

            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = Order;
            using pointer = const Order*;
            using reference = const Order&;
            using iterator_category = std::forward_iterator_tag;
        };

        uint32_t _beginTableOffset;
        uint32_t _currentOrderOffset;

    public:
        // currentOrderOffset is relative to beginTableOffset and is where the ring will begin and end
        OrderRingView(uint32_t beginTableOffset, uint32_t currentOrderOffset = 0)
            : _beginTableOffset(beginTableOffset)
            , _currentOrderOffset(currentOrderOffset)
        {
        }

        OrderRingView::Iterator begin() const;
        OrderRingView::Iterator end() const;

        Order* atIndex(const uint8_t index) const;
    };

    void zeroOrderTable();
}
