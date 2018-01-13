#pragma once

#include <string>
#include "interop/interop.hpp"

namespace openloco::ui
{
    extern loco_global<void *, 0x00525320> hwnd;

    int32_t width();
    int32_t height();

    void create_window();
    void initialise();
    void initialise_cursors();
    void initialise_input();
    void dispose_input();
    void update();
    void render();
    bool process_messages();
    void show_message_box(const std::string &title, const std::string &message);
    std::string prompt_directory(const std::string &title);
}
