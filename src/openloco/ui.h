#pragma once

#include <string>

namespace openloco::ui
{
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

#ifdef _WIN32
    void* hwnd();
#endif
    int32_t width();
    int32_t height();

    void create_window();
    void initialise();
    void initialise_cursors();
    void initialise_input();
    void dispose_input();
    void dispose_cursors();
    void set_cursor(cursor_id id);
    void get_cursor_pos(int32_t& x, int32_t& y);
    void set_cursor_pos(int32_t x, int32_t y);
    void hide_cursor();
    void show_cursor();
    void update();
    void render();
    bool process_messages();
    void show_message_box(const std::string& title, const std::string& message);
}
