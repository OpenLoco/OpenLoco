#pragma once

#include "../Core/Span.hpp"
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
    stdx::span<tile_element> getElements();
    tile_element* getElementsEnd();
    tile_element** getElementIndex();
    tile get(map_pos pos);
    tile get(coord_t x, coord_t y);
    TileHeight getHeight(const map_pos& pos);
    void reorganise();
    void mapInvalidateSelectionRect();
    void mapInvalidateTileFull(Map::map_pos pos);
    void mapInvalidateMapSelectionTiles();
}
