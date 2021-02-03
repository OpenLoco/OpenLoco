#include "Orders.h"
#include "Map/Tile.h"

namespace OpenLoco::Vehicle
{
    // 0x004FE070
    static constexpr uint8_t OrderSizes[] = {
        sizeof(OrderEnd),
        sizeof(OrderStopAt),
        sizeof(OrderRouteThrough),
        sizeof(OrderRouteWaypoint),
        sizeof(OrderUnloadAll),
        sizeof(OrderWaitFor),
    };

    Map::map_pos3& OrderRouteWaypoint::getWaypoint() const
    {
        Map::map_pos3 loc{};
        loc.x = ((static_cast<int16_t>(_type & 0x80) << 1) | _1) * Map::tile_size + 16;
        loc.y = ((static_cast<int16_t>(_2 & 0x80) << 1) | _3) * Map::tile_size + 16;
        loc.z = (_2 & 0x7F) * 8 + 32;
        return loc;
    }

}