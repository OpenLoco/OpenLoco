#pragma once

#include "map/tile.h"
#include "Station.h"
#include "things/thing.h"
#include "Types.hpp"
#include "Window.h"
#include <array>

namespace openloco::ui::viewportmgr
{
    constexpr int16_t viewportsPerWindow = 2;

    void init();
    void registerHooks();
    void collectGarbage();
    viewport* create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom, thing_id_t thing_id);
    viewport* create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom, map::map_pos3 tile);
    void invalidate(station* station);
    void invalidate(Thing* t, ZoomLevel zoom);
    void invalidate(map::map_pos pos, coord_t zMin, coord_t zMax, ZoomLevel zoom = ZoomLevel::eighth, int radius = 32);
}
