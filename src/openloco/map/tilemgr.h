#pragma once

#include "tile.h"
#include <cstdint>
#include <tuple>

namespace openloco::map
{
    class tilemanager
    {
    public:
        tile get(map_pos pos);
        const tile get(map_pos pos) const;
        tile get(coord_t x, coord_t y);
        const tile get(coord_t x, coord_t y) const;
        std::tuple<int16_t, int16_t> get_height(coord_t x, coord_t y) const;
    };

    extern tilemanager g_tilemgr;
}
