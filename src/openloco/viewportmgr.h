#pragma once

#include "Window.h"
#include <array>

namespace openloco::ui::viewportmgr
{
    constexpr size_t max_viewports = 10;
    std::array<viewport*, max_viewports> viewports();
    void create(Window* window, int16_t x, int16_t y, uint16_t width, uint16_t height, bool zoom_flag, uint8_t zoom, uint16_t thing_id);
    void create(Window* window, int16_t x, int16_t y, uint16_t width, uint16_t height, bool zoom_flag, uint8_t zoom, uint16_t tile_x, uint16_t tile_y, uint16_t tile_z);
}
