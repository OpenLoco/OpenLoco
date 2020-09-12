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

    namespace MapSelectionFlags
    {
        constexpr uint8_t unk_04 = 1 << 4; // Vehicle orders?
        constexpr uint8_t catchment_area = 1 << 5;
    };

    namespace KeyModifier
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
    bool isHovering(Ui::WindowType);
    bool isHovering(Ui::WindowType, Ui::window_number);
    bool isHovering(Ui::WindowType type, Ui::window_number number, Ui::widget_index widgetIndex);
    Ui::widget_index getHoveredWidgetIndex();

    bool isDropdownActive(Ui::WindowType type, Ui::widget_index index);

    bool isPressed(Ui::WindowType type, Ui::window_number number);
    bool isPressed(Ui::WindowType type, Ui::window_number number, Ui::widget_index index);
    Ui::widget_index getPressedWidgetIndex();
    void setPressedWidgetIndex(Ui::widget_index index);

    void updateCursorPosition();

    Ui::window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType, Ui::window_number);
    bool isToolActive(Ui::WindowType, Ui::window_number, int16_t);
    bool toolSet(Ui::window* w, int16_t widgetIndex, uint8_t tool);
    void toolCancel();
    void toolCancel(Ui::WindowType, Ui::window_number);

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

    void windowPositionBegin(int16_t x, int16_t y, Ui::window* window, Ui::widget_index widget_index);

    Gfx::point_t getScrollLastLocation();
    Gfx::point_t getDragLastLocation();
    Gfx::point_t getTooltipMouseLocation();
    void setTooltipMouseLocation(const Gfx::point_t& loc);
    uint16_t getTooltipTimeout();
    void setTooltipTimeout(uint16_t tooltipTimeout);

    void setClickRepeatTicks(uint16_t ticks);
}
