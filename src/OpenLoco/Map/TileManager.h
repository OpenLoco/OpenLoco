#pragma once

#include "Tile.h"
#include <cstdint>
#include <tuple>

namespace OpenLoco::Map::tilemgr
{
    enum MapSelectFlag : uint16_t
    {
        enable = (1 << 0),
        enableConstruct = (1 << 1)
    };

    tile get(map_pos pos);
    tile get(coord_t x, coord_t y);
    std::tuple<int16_t, int16_t> getHeight(coord_t x, coord_t y);
    void mapInvalidateSelectionRect();
    void mapInvalidateTileFull(Map::map_pos pos);
    void mapInvalidateMapSelectionTiles();
}
