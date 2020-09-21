#pragma once

#include "Ui/WindowManager.h"
#include "Window.h"

namespace OpenLoco::Input
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
    void initMouse();
    bool hasFlag(input_flags value);
    void setFlag(input_flags value);
    void resetFlag(input_flags value);
    input_state state();
    void state(input_state);

    Gfx::point_t getMouseLocation();
    bool isHovering(ui::WindowType);
    bool isHovering(ui::WindowType, ui::window_number);
    bool isHovering(ui::WindowType type, ui::window_number number, ui::widget_index widgetIndex);
    ui::widget_index getHoveredWidgetIndex();

    bool isDropdownActive(ui::WindowType type, ui::widget_index index);

    bool isPressed(ui::WindowType type, ui::window_number number);
    bool isPressed(ui::WindowType type, ui::window_number number, ui::widget_index index);
    ui::widget_index getPressedWidgetIndex();

    void updateCursorPosition();

    bool isToolActive(ui::WindowType, ui::window_number);
    bool toolSet(ui::window* w, int16_t widgetIndex, uint8_t tool);
    void toolCancel();
    void toolCancel(ui::WindowType, ui::window_number);

    bool hasKeyModifier(uint8_t modifier);
    uint16_t getMapSelectionFlags();
    bool hasMapSelectionFlag(uint8_t flags);
    void setMapSelectionFlags(uint8_t flags);
    void resetMapSelectionFlag(uint8_t flags);

    void handleKeyboard();
    void handleMouse(int16_t x, int16_t y, mouse_button button);
    mouse_button getLastKnownButtonState();
    void enqueueMouseButton(int32_t button);
    void moveMouse(int32_t x, int32_t y, int32_t relX, int32_t relY);
    void sub_407218();
    void sub_407231();
    void processMouseOver(int16_t x, int16_t y);
    void processKeyboardInput();

    Gfx::point_t getTooltipMouseLocation();
    void setTooltipMouseLocation(const Gfx::point_t& loc);
    uint16_t getTooltipTimeout();
    void setTooltipTimeout(uint16_t tooltipTimeout);
}
