#include "../interop/interop.hpp"
#include "tilemgr.h"

using namespace openloco::interop;

namespace openloco::map::tilemgr
{
    static loco_global_array<tile_element*, 0x30004, 0x00E40134> _tiles;

    tile get(coord_t x, coord_t y)
    {
        tile_coord_t tileX = x / 32;
        tile_coord_t tileY = y / 32;

        size_t index = ((y << 9) | x) >> 5;
        return tile(tileX, tileY, _tiles[index]);
    }
}
