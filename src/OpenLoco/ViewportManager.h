#pragma once

#include "Map/Tile.h"
#include "Station.h"
#include "Things/Thing.h"
#include "Types.hpp"
#include "Window.h"
#include <array>

namespace OpenLoco::ui::viewportmgr
{
    constexpr int16_t viewportsPerWindow = 2;

    void init();
    void registerHooks();
    void collectGarbage();
    viewport* create(window* window, int viewportIndex, Gfx::point_t origin, Gfx::ui_size_t size, ZoomLevel zoom, thing_id_t thing_id);
    viewport* create(window* window, int viewportIndex, Gfx::point_t origin, Gfx::ui_size_t size, ZoomLevel zoom, map::map_pos3 tile);
    void invalidate(station* station);
    void invalidate(Thing* t, ZoomLevel zoom);
    void invalidate(map::map_pos pos, coord_t zMin, coord_t zMax, ZoomLevel zoom = ZoomLevel::eighth, int radius = 32);
}
