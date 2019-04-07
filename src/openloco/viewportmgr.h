#pragma once

#include "map/tile.h"
#include "station.h"
#include "things/thing.h"
#include "window.h"
#include <array>

namespace openloco::ui::viewportmgr
{
    enum class ZoomLevel : uint8_t
    {
        full = 0,
        half = 1,
        quarter = 2,
        eighth = 3,
    };

    constexpr int16_t viewportsPerWindow = 2;

    void init();
    void registerHooks();
    void collectGarbage();
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom, thing_id_t thing_id);
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom, map::map_pos3 tile);
    void invalidate(station* station);
    void invalidate(Thing* t, ZoomLevel zoom);
    void invalidate(map::map_pos pos, map::coord_t zMin, map::coord_t zMax, ZoomLevel zoom = ZoomLevel::eighth, int radius = 32);
}
