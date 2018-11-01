#pragma once

#include "ui/WindowManager.h"

namespace openloco::input
{
    enum class mouse_button
    {
        released,
        left_pressed,
        left_released,
        right_pressed,
        right_released,
    };

    enum class input_state
    {
        reset,              // 0
        normal,             // 1
        widget_pressed,     // 2
        positioning_window, // 3
        viewport_right,     // 4
        dropdown_active,    // 5
        viewport_left,      // 6
        scroll_left,        // 7
        resizing,           // 8
        scroll_right,       // 9
    };

    enum class input_flags
    {
        widget_pressed = 1 << 0,
        flag1 = 1 << 1,
        flag2 = 1 << 2,
        tool_active = 1 << 3,
        flag4 = 1 << 4,
        flag5 = 1 << 5,
        viewport_scrolling = 1 << 7,
    };

    enum class scroll_flags
    {
        HSCROLLBAR_VISIBLE = (1 << 0),
        HSCROLLBAR_THUMB_PRESSED = (1 << 1),
        HSCROLLBAR_LEFT_PRESSED = (1 << 2),
        HSCROLLBAR_RIGHT_PRESSED = (1 << 3),
        VSCROLLBAR_VISIBLE = (1 << 4),
        VSCROLLBAR_THUMB_PRESSED = (1 << 5),
        VSCROLLBAR_UP_PRESSED = (1 << 6),
        VSCROLLBAR_DOWN_PRESSED = (1 << 7),
    };

    namespace key_modifier
    {
        constexpr uint8_t shift = 1 << 0;
        constexpr uint8_t control = 1 << 1;
        constexpr uint8_t cheat = 1 << 7;
    };

    void init();
    void init_mouse();
    bool has_flag(input_flags value);
    void set_flag(input_flags value);
    void reset_flag(input_flags value);
    input_state state();
    void state(input_state);

    bool is_hovering(ui::WindowType);
    bool is_hovering(ui::WindowType, ui::window_number);
    ui::widget_index get_hovered_widget_index();

    bool is_pressed(ui::WindowType type, ui::window_number number, ui::widget_index index);

    bool is_tool_active(ui::WindowType, ui::window_number);
    void cancel_tool();
    void cancel_tool(ui::WindowType, ui::window_number);

    bool has_key_modifier(uint8_t modifier);

    void handle_keyboard();
    void handle_mouse(int16_t x, int16_t y, mouse_button button);
    void enqueue_mouse_button(mouse_button button);
    void move_mouse(int32_t x, int32_t y, int32_t relX, int32_t relY);
    void sub_407218();
    void sub_407231();
    void process_mouse_over(int16_t x, int16_t y);
    void process_keyboard_input();
}
