#pragma once

#include "tile.h"
#include <cstdint>
#include <tuple>

namespace openloco::map::tilemgr
{
    tile get(map_pos pos);
    tile get(coord_t x, coord_t y);
    std::tuple<int16_t, int16_t> get_height(coord_t x, coord_t y);
    void map_invalidate_selection_rect();
    void map_invalidate_tile_full(map::map_pos pos);
    void map_invalidate_map_selection_tiles();
}
