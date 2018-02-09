#pragma once

#include <cstdint>

namespace openloco::input
{
    enum class mouse_button
    {
        left_down = 1,
        right_down,
        left_up,
        right_up,
    };

    enum class input_state
    {
        reset,
        normal,
        widget_pressed,
        positioning_window,
        viewport_right,
        dropdown_active,
        viewport_left,
        scroll_left,
        resizing,
        scroll_right,
    };

    enum class input_flags
    {
        flag_3 = 1 << 3,
        viewport_scrolling = 1 << 7,
    };

    enum class key_modifier
    {
        shift = 1 << 0,
    };

    bool has_flag(input_flags value);
    void set_flag(input_flags value);
    void reset_flag(input_flags value);
    input_state state();

    bool has_key_modifier(key_modifier modifier);

    void handle_keyboard();
    void enqueue_mouse_button(mouse_button button);
    void move_mouse(int32_t x, int32_t y, int32_t relX, int32_t relY);
    void sub_407218();
    void sub_407231();
    void process_mouse_over(int16_t x, int16_t y);
}
