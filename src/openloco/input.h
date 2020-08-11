#pragma once

#include "ui/WindowManager.h"
#include "window.h"

namespace openloco::input
{
    enum class mouse_button : uint16_t
    {
        released = 0,
        left_pressed = 1,
        left_released = 2,
        right_pressed = 3,
        right_released = 4,
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
        flag6 = 1 << 6,
        viewport_scrolling = 1 << 7,
    };

    namespace map_selection_flags
    {
        constexpr uint8_t catchment_area = 1 << 5;
    };

    namespace key_modifier
    {
        constexpr uint8_t shift = 1 << 0;
        constexpr uint8_t control = 1 << 1;
        constexpr uint8_t unknown = 1 << 2;
        constexpr uint8_t cheat = 1 << 7;
    };

    void init();
    void init_mouse();
    bool has_flag(input_flags value);
    void set_flag(input_flags value);
    void reset_flag(input_flags value);
    input_state state();
    void state(input_state);

    gfx::point_t getMouseLocation();
    bool is_hovering(ui::WindowType);
    bool is_hovering(ui::WindowType, ui::window_number);
    bool is_hovering(ui::WindowType type, ui::window_number number, ui::widget_index widgetIndex);
    ui::widget_index get_hovered_widget_index();

    bool is_dropdown_active(ui::WindowType type, ui::widget_index index);

    bool is_pressed(ui::WindowType type, ui::window_number number);
    bool is_pressed(ui::WindowType type, ui::window_number number, ui::widget_index index);
    ui::widget_index get_pressed_widget_index();

    void updateCursorPosition();

    bool is_tool_active(ui::WindowType, ui::window_number);
    bool toolSet(ui::window* w, int16_t widgetIndex, uint8_t tool);
    void cancel_tool();
    void cancel_tool(ui::WindowType, ui::window_number);

    bool has_key_modifier(uint8_t modifier);
    uint16_t getMapSelectionFlags();
    bool hasMapSelectionFlag(uint8_t flags);
    void setMapSelectionFlags(uint8_t flags);
    void resetMapSelectionFlag(uint8_t flags);

    void handle_keyboard();
    void handle_mouse(int16_t x, int16_t y, mouse_button button);
    mouse_button getLastKnownButtonState();
    void enqueue_mouse_button(int32_t button);
    void move_mouse(int32_t x, int32_t y, int32_t relX, int32_t relY);
    void sub_407218();
    void sub_407231();
    void process_mouse_over(int16_t x, int16_t y);
    void process_keyboard_input();

    gfx::point_t getTooltipMouseLocation();
    void setTooltipMouseLocation(const gfx::point_t& loc);
    uint16_t getTooltipTimeout();
    void setTooltipTimeout(uint16_t tooltipTimeout);
}
