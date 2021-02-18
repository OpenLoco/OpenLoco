#pragma once

#include "Tile.h"
#include <cstdint>
#include <tuple>

namespace OpenLoco::Map::TileManager
{
    enum MapSelectFlag : uint16_t
    {
        enable = (1 << 0),
        enableConstruct = (1 << 1)
    };

    void initialise();
    tile get(map_pos pos);
    tile get(coord_t x, coord_t y);
    TileHeight getHeight(const map_pos& pos);
    void mapInvalidateSelectionRect();
    void mapInvalidateTileFull(Map::map_pos pos);
    void mapInvalidateMapSelectionTiles();
}
