#include "Orders.h"
#include "Map/Tile.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Vehicles/OrderManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x004FE088 TODO: Rework into class
    static constexpr OrderFlags kOrderFlags[] = {
        OrderFlags::none,
        OrderFlags::IsRoutable | OrderFlags::HasNumber | OrderFlags::HasStation,
        OrderFlags::IsRoutable | OrderFlags::HasNumber | OrderFlags::HasStation,
        OrderFlags::IsRoutable | OrderFlags::HasNumber,
        OrderFlags::HasCargo,
        OrderFlags::HasCargo,
    };

    bool Order::hasFlags(const OrderFlags flag) const
    {
        return (kOrderFlags[static_cast<uint8_t>(getType())] & flag) != OrderFlags::none;
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

    void OrderRouteWaypoint::setWaypoint(const World::TilePos2& pos, const uint8_t baseZ)
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

    World::Pos3 OrderRouteWaypoint::getWaypoint() const
    {
        World::Pos3 loc{};
        loc.x = ((static_cast<int16_t>(_type & 0x80) << 1) | _data[0]) * World::kTileSize;
        loc.y = ((static_cast<int16_t>(_data[1] & 0x80) << 1) | _data[2]) * World::kTileSize;
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
        return this - OrderManager::orders();
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

    uint8_t swapAdjacentOrders(Order& a, Order& b)
    {
        const auto rawOrderA = a.getRaw();
        const auto rawOrderB = b.getRaw();
        const auto lengthOrderA = kOrderSizes[enumValue(a.getType())];
        const auto lengthOrderB = kOrderSizes[enumValue(b.getType())];

        // Ensure the two orders are indeed adjacent
        assert(&a + lengthOrderA == &b);

        // Copy B over A, and append B right after
        const auto dest = reinterpret_cast<uint8_t*>(&a);
        std::memcpy(dest, &rawOrderB, lengthOrderB);
        std::memcpy(dest + lengthOrderB, &rawOrderA, lengthOrderA);

        // Return the length with which to offset
        return lengthOrderB;
    }
}
