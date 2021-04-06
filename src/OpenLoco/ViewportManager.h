#pragma once

#include "Entities/Entity.h"
#include "Map/Tile.h"
#include "Station.h"
#include "Types.hpp"
#include "Window.h"
#include <array>

namespace OpenLoco::Ui::ViewportManager
{
    constexpr int16_t viewportsPerWindow = 2;

    void init();
    void registerHooks();
    void collectGarbage();
    viewport* create(window* window, int viewportIndex, Gfx::point_t origin, Gfx::ui_size_t size, ZoomLevel zoom, EntityId_t thing_id);
    viewport* create(window* window, int viewportIndex, Gfx::point_t origin, Gfx::ui_size_t size, ZoomLevel zoom, Map::map_pos3 tile);
    void invalidate(Station* station);
    void invalidate(EntityBase* t, ZoomLevel zoom);
    void invalidate(Map::map_pos pos, coord_t zMin, coord_t zMax, ZoomLevel zoom = ZoomLevel::eighth, int radius = 32);
}
