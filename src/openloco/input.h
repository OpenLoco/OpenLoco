#pragma once

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
        viewport_scrolling = 1 << 7
    };

    bool has_flag(input_flags value);
    void set_flag(input_flags value);
    void reset_flag(input_flags value);
    input_state state();

    void handle_keyboard();
    void enqueue_mouse_button(mouse_button button);
    void sub_407218();
    void sub_407231();
}
