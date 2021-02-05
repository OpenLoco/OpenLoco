#include "Orders.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Map/Tile.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "StationManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicle
{
    constexpr uint32_t max_orders = 256000;
    static loco_global<Order[max_orders], 0x00987C5C> _orderTable;

    // 0x004FE070
    static constexpr uint8_t OrderSizes[] = {
        sizeof(OrderEnd),
        sizeof(OrderStopAt),
        sizeof(OrderRouteThrough),
        sizeof(OrderRouteWaypoint),
        sizeof(OrderUnloadAll),
        sizeof(OrderWaitFor),
    };

    std::shared_ptr<Order> Order::clone() const
    {
        switch (getType())
        {
            case OrderType::End:
            {
                auto cloneOrder = std::make_shared<OrderEnd>();
                cloneOrder->setType(OrderType::End);
                return cloneOrder;
            }
            case OrderType::StopAt:
            {
                auto* stopOrder = as<OrderStopAt>();
                if (stopOrder != nullptr)
                {
                    auto cloneOrder = std::make_shared<OrderStopAt>();
                    cloneOrder->setType(OrderType::StopAt);
                    cloneOrder->setStation(stopOrder->getStation());
                    return cloneOrder;
                }
                break;
            }
            case OrderType::RouteThrough:
            {

                auto* routeOrder = as<OrderRouteThrough>();
                if (routeOrder != nullptr)
                {
                    auto cloneOrder = std::make_shared<OrderRouteThrough>();
                    cloneOrder->setType(OrderType::RouteThrough);
                    cloneOrder->setStation(routeOrder->getStation());
                    return cloneOrder;
                }
                break;
            }
            case OrderType::RouteWaypoint:
            {
                auto* waypointOrder = as<OrderRouteWaypoint>();
                if (waypointOrder != nullptr)
                {
                    auto cloneOrder = std::make_shared<OrderRouteWaypoint>();
                    cloneOrder->setType(OrderType::RouteWaypoint);
                    tile_coord_t x = ((waypointOrder->_type & 0x80) << 1) | waypointOrder->_1;
                    tile_coord_t y = ((waypointOrder->_2 & 0x80) << 1) | waypointOrder->_3;
                    auto z = (waypointOrder->_2 & 0x7F);
                    cloneOrder->setWaypoint({ x, y }, z);
                    cloneOrder->setDirection(waypointOrder->getDirection());
                    cloneOrder->setTrackId(waypointOrder->getTrackId());
                    return cloneOrder;
                }
                break;
            }
            case OrderType::UnloadAll:
            {
                auto* unloadOrder = as<OrderUnloadAll>();
                if (unloadOrder != nullptr)
                {
                    auto cloneOrder = std::make_shared<OrderUnloadAll>();
                    cloneOrder->setType(OrderType::UnloadAll);
                    cloneOrder->setCargo(unloadOrder->getCargo());
                    return cloneOrder;
                }
                break;
            }
            case OrderType::WaitFor:
            {
                auto* waitOrder = as<OrderWaitFor>();
                if (waitOrder != nullptr)
                {
                    auto cloneOrder = std::make_shared<OrderWaitFor>();
                    cloneOrder->setType(OrderType::WaitFor);
                    cloneOrder->setCargo(waitOrder->getCargo());
                    return cloneOrder;
                }
                break;
            }
        }
        return {};
    }

    void OrderRouteWaypoint::setWaypoint(const Map::TilePos& pos, const uint8_t baseZ)
    {
        _1 &= ~0x80;
        _1 |= ((pos.x & 0x100) >> 1);
        _2 = pos.x & 0xFF;
        _4 = pos.y & 0xFF;
        _3 = baseZ;
        _3 |= ((pos.y & 0x100) >> 1);
    }

    void OrderRouteWaypoint::setDirection(const uint8_t direction)
    {
        _4 &= ~0x7;
        _4 |= direction & 0x7;
    }

    void OrderRouteWaypoint::setTrackId(const uint8_t trackId)
    {
        _4 &= ~0xF8;
        _4 = (trackId & 0x1F) << 3;
        _5 = (trackId >> 5) & 0x1;
    }

    Map::map_pos3 OrderRouteWaypoint::getWaypoint() const
    {
        Map::map_pos3 loc{};
        loc.x = ((static_cast<int16_t>(_type & 0x80) << 1) | _1) * Map::tile_size + 16;
        loc.y = ((static_cast<int16_t>(_2 & 0x80) << 1) | _3) * Map::tile_size + 16;
        loc.z = (_2 & 0x7F) * 8 + 32;
        return loc;
    }

    uint8_t OrderRouteWaypoint::getDirection() const
    {
        return _4 & 0x7;
    }

    uint8_t OrderRouteWaypoint::getTrackId() const
    {
        return (_4 >> 3) | ((_5 & 0x1) << 5);
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
                    ret |= route->_1 << 8;
                }
                break;
            }
            case OrderType::RouteWaypoint:
            {
                auto* route = as<OrderRouteWaypoint>();
                if (route != nullptr)
                {
                    ret |= route->_1 << 8;
                    ret |= route->_2 << 16;
                    ret |= route->_3 << 24;
                    ret |= static_cast<uint64_t>(route->_4) << 32;
                    ret |= static_cast<uint64_t>(route->_5) << 40;
                }
                break;
            }
            case OrderType::StopAt:
            {
                auto* stop = as<OrderStopAt>();
                if (stop != nullptr)
                {
                    ret |= stop->_1 << 8;
                }
                break;
            }
        }
        return ret;
    }

    OrderTableView::Iterator& OrderTableView::Iterator::operator++()
    {
        auto* newOrders = reinterpret_cast<uint8_t*>(_orders) + OrderSizes[static_cast<uint8_t>(_orders->getType())];
        _orders = reinterpret_cast<Order*>(newOrders);
        return *this;
    }

    OrderTableView::Iterator OrderTableView::begin()
    {
        return Iterator(&_orderTable[_beginOffset]);
    }

    OrderTableView::Iterator OrderTableView::end()
    {
        static OrderEnd _end;
        return Iterator(&_end);
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
        auto cargoObj = ObjectManager::get<cargo_object>(getCargo());
        args.push(cargoObj->name);
        args.push(cargoObj->unit_inline_sprite);
    }

}
