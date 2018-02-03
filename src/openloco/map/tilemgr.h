#pragma once

#include "tile.h"
#include <cstdint>
#include <tuple>

namespace openloco::map::tilemgr
{
    tile get(map_pos pos);
    tile get(coord_t x, coord_t y);
    std::tuple<int16_t, int16_t> get_height(coord_t x, coord_t y);
}
