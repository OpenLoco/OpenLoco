#include "Orders.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"

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

    Map::map_pos3 OrderRouteWaypoint::getWaypoint() const
    {
        Map::map_pos3 loc{};
        loc.x = ((static_cast<int16_t>(_type & 0x80) << 1) | _1) * Map::tile_size + 16;
        loc.y = ((static_cast<int16_t>(_2 & 0x80) << 1) | _3) * Map::tile_size + 16;
        loc.z = (_2 & 0x7F) * 8 + 32;
        return loc;
    }

    uint32_t Order::getOffset() const
    {
        return this - _orderTable;
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

}
