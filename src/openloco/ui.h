#pragma once

#include "interop/interop.hpp"

namespace openloco::ui
{
    extern loco_global<void *, 0x00525320> hwnd;

    int32_t width();
    int32_t height();

    void create_window();
    void initialise();
    void update();
    void render();
    bool process_messages();
}
