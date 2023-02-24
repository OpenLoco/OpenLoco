#pragma once

#include "Entities/Entity.h"
#include "Map/Tile.h"
#include "Types.hpp"
#include "Window.h"
#include "World/Station.h"
#include <array>

namespace OpenLoco::Ui::ViewportManager
{
    constexpr int16_t viewportsPerWindow = 2;

    void init();
    void registerHooks();
    void collectGarbage();
    Viewport* create(Window* window, int viewportIndex, Ui::Point origin, Ui::Size size, ZoomLevel zoom, EntityId thing_id);
    Viewport* create(Window* window, int viewportIndex, Ui::Point origin, Ui::Size size, ZoomLevel zoom, World::Pos3 tile);
    void invalidate(Station* station);
    void invalidate(EntityBase* t, ZoomLevel zoom);
    void invalidate(World::Pos2 pos, coord_t zMin, coord_t zMax, ZoomLevel zoom = ZoomLevel::eighth, int radius = 32);
}
