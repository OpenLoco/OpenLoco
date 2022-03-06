#include "Orders.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Map/Tile.h"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../StationManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<Order[Limits::kMaxOrders], 0x00987C5C> _orderTable;

    // 0x004FE070
    static constexpr uint8_t kOrderSizes[] = {
        sizeof(OrderEnd),
        sizeof(OrderStopAt),
        sizeof(OrderRouteThrough),
        sizeof(OrderRouteWaypoint),
        sizeof(OrderUnloadAll),
        sizeof(OrderWaitFor),
    };

    // 0x004FE088 TODO: Rework into class
    static constexpr uint8_t kOrderFlags[] = {
        0,
        OrderFlags::IsRoutable | OrderFlags::HasNumber | OrderFlags::HasStation,
        OrderFlags::IsRoutable | OrderFlags::HasNumber | OrderFlags::HasStation,
        OrderFlags::IsRoutable | OrderFlags::HasNumber,
        OrderFlags::HasCargo,
        OrderFlags::HasCargo,
    };

    bool Order::hasFlag(const uint8_t flag) const
    {
        return kOrderFlags[static_cast<uint8_t>(getType())] & flag;
    }

    template<typename T>
    std::shared_ptr<Order> orderClone(const Order& order)
    {
        auto* o = order.as<T>();
        return o != nullptr ? std::make_shared<T>(*o) : nullptr;
    }

    std::shared_ptr<Order> Order::clone() const
    {
        switch (getType())
        {
            case OrderType::End:
                return orderClone<OrderEnd>(*this);
            case OrderType::StopAt:
                return orderClone<OrderStopAt>(*this);
            case OrderType::RouteThrough:
                return orderClone<OrderRouteThrough>(*this);
            case OrderType::RouteWaypoint:
                return orderClone<OrderRouteWaypoint>(*this);
            case OrderType::UnloadAll:
                return orderClone<OrderUnloadAll>(*this);
            case OrderType::WaitFor:
                return orderClone<OrderWaitFor>(*this);
        }
        return {};
    }

    void OrderRouteWaypoint::setWaypoint(const Map::TilePos2& pos, const uint8_t baseZ)
    {
        _type &= ~0x80;
        _type |= ((pos.x & 0x100) >> 1);
        _data[0] = pos.x & 0xFF;
        _data[2] = pos.y & 0xFF;
        _data[1] = baseZ;
        _data[1] |= ((pos.y & 0x100) >> 1);
    }

    void OrderRouteWaypoint::setDirection(const uint8_t direction)
    {
        _data[3] &= ~0x7;
        _data[3] |= direction & 0x7;
    }

    void OrderRouteWaypoint::setTrackId(const uint8_t trackId)
    {
        _data[3] &= ~0xF8;
        _data[3] |= (trackId & 0x1F) << 3;
        _data[4] = (trackId >> 5) & 0x1;
    }

    Map::Pos3 OrderRouteWaypoint::getWaypoint() const
    {
        Map::Pos3 loc{};
        loc.x = ((static_cast<int16_t>(_type & 0x80) << 1) | _data[0]) * Map::tile_size;
        loc.y = ((static_cast<int16_t>(_data[1] & 0x80) << 1) | _data[2]) * Map::tile_size;
        loc.z = (_data[1] & 0x7F) * 8;
        return loc;
    }

    uint8_t OrderRouteWaypoint::getDirection() const
    {
        return _data[3] & 0x7;
    }

    uint8_t OrderRouteWaypoint::getTrackId() const
    {
        return (_data[3] >> 3) | ((_data[4] & 0x1) << 5);
    }

    uint32_t Order::getOffset() const
    {
        return this - _orderTable;
    }

    uint64_t Order::getRaw() const
    {
        uint64_t ret = 0;
        ret = _type;
        switch (getType())
        {
            case OrderType::RouteThrough:
            {
                auto* route = as<OrderRouteThrough>();
                if (route != nullptr)
                {
                    ret |= static_cast<uint64_t>(route->_data) << 8;
                }
                break;
            }
            case OrderType::RouteWaypoint:
            {
                auto* route = as<OrderRouteWaypoint>();
                if (route != nullptr)
                {
                    ret |= static_cast<uint64_t>(route->_data[0]) << 8;
                    ret |= static_cast<uint64_t>(route->_data[1]) << 16;
                    ret |= static_cast<uint64_t>(route->_data[2]) << 24;
                    ret |= static_cast<uint64_t>(route->_data[3]) << 32;
                    ret |= static_cast<uint64_t>(route->_data[4]) << 40;
                }
                break;
            }
            case OrderType::StopAt:
            {
                auto* stop = as<OrderStopAt>();
                if (stop != nullptr)
                {
                    ret |= static_cast<uint64_t>(stop->_data) << 8;
                }
                break;
            }
            case OrderType::End:
            case OrderType::UnloadAll:
            case OrderType::WaitFor:
                // These are all one byte and no more data to save
                break;
        }
        return ret;
    }

    // 0x004B49F8
    void OrderStation::setFormatArguments(FormatArguments& args) const
    {
        auto station = StationManager::get(getStation());
        args.push(station->name);
        args.push(station->town);
    }

    // 0x004B4A31
    void OrderCargo::setFormatArguments(FormatArguments& args) const
    {
        auto cargoObj = ObjectManager::get<CargoObject>(getCargo());
        args.push(cargoObj->name);
        args.push(cargoObj->unit_inline_sprite);
    }

    Order* OrderRingView::atIndex(const uint8_t index) const
    {
        auto size = std::distance(begin(), end());
        if (index >= size)
        {
            return nullptr;
        }
        auto chosenOrder = std::next(begin(), index);
        return &(*chosenOrder);
    }

    OrderRingView::Iterator& OrderRingView::Iterator::operator++()
    {
        auto* newOrders = reinterpret_cast<uint8_t*>(_currentOrder) + kOrderSizes[static_cast<uint8_t>(_currentOrder->getType())];
        _currentOrder = reinterpret_cast<Order*>(newOrders);
        if (_currentOrder->getType() == OrderType::End)
        {
            _currentOrder = _beginOrderTable;
            _hasLooped = true;
        }
        return *this;
    }

    OrderRingView::Iterator OrderRingView::begin() const
    {
        return Iterator(&_orderTable[_beginTableOffset], &_orderTable[_beginTableOffset + _currentOrderOffset]);
    }

    OrderRingView::Iterator OrderRingView::end() const
    {
        return begin();
    }

    // 0x004702F7
    void zeroOrderTable()
    {
        call(0x004702F7);
    }
}
