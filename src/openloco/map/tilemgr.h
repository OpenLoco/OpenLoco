#pragma once

#include "tile.h"
#include <cstdint>

namespace openloco::map::tilemgr
{
    tile get(coord_t x, coord_t y);
}
