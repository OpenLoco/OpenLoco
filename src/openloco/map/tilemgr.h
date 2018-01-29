#pragma once

#include "tile.h"
#include <cstdint>

namespace openloco::map::tilemgr
{
    tile get(map_pos pos);
    tile get(coord_t x, coord_t y);
}
