#pragma once

#include <string>
#include "interop/interop.hpp"

namespace openloco::ui
{
    extern loco_global<void *, 0x00525320> hwnd;

    enum class cursor_id
    {
        pointer,
        blank,
        up_arrow,
        up_down_arrow,
        hand_pointer,
        busy,
        diagonal_arrows,
    };

    int32_t width();
    int32_t height();

    void create_window();
    void initialise();
    void initialise_cursors();
    void initialise_input();
    void dispose_input();
    void dispose_cursors();
    void set_cursor(cursor_id id);
    void update();
    void render();
    bool process_messages();
    void show_message_box(const std::string &title, const std::string &message);
    std::string prompt_directory(const std::string &title);
}
