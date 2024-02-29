#pragma once

#include "Ui/WindowManager.h"
#include "Window.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Point.hpp>

namespace OpenLoco::Input
{
    enum class MouseButton : uint16_t
    {
        released = 0,
        leftPressed = 1,
        leftReleased = 2,
        rightPressed = 3,
        rightReleased = 4,
    };

    enum class State : uint8_t
    {
        reset,             // 0
        normal,            // 1
        widgetPressed,     // 2
        positioningWindow, // 3
        viewportRight,     // 4
        dropdownActive,    // 5
        viewportLeft,      // 6
        scrollLeft,        // 7
        resizing,          // 8
        scrollRight,       // 9
    };

    enum class Flags : uint32_t
    {
        none = 0U,
        widgetPressed = 1U << 0,
        flag1 = 1U << 1,
        flag2 = 1U << 2,
        toolActive = 1U << 3,
        leftMousePressed = 1U << 4,
        rightMousePressed = 1U << 5,
        flag6 = 1U << 6,
        viewportScrolling = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags);

    enum class KeyModifier : uint8_t
    {
        none = 0U,
        shift = 1U << 0,
        control = 1U << 1,
        unknown = 1U << 2,
        cheat = 1U << 7,
        invalid = 0xFF,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(KeyModifier);

    void init();
    void initMouse();
    bool hasFlag(Flags value);
    void setFlag(Flags value);
    void resetFlag(Flags value);
    State state();
    void state(State);

    Ui::Point getMouseLocation();
    Ui::Point getMouseLocation2();
    bool isHovering(Ui::WindowType);
    bool isHovering(Ui::WindowType, Ui::WindowNumber_t);
    bool isHovering(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t widgetIndex);
    Ui::WidgetIndex_t getHoveredWidgetIndex();

    bool isDropdownActive(Ui::WindowType type, Ui::WidgetIndex_t index);

    bool isPressed(Ui::WindowType type, Ui::WindowNumber_t number);
    bool isPressed(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t index);
    Ui::WidgetIndex_t getPressedWidgetIndex();
    void setPressedWidgetIndex(Ui::WidgetIndex_t index);

    void updateCursorPosition();

    void enqueueText(const char* text);
    void enqueueKey(uint32_t key);
    bool hasKeyModifier(KeyModifier modifier);

    StationId getHoveredStationId();

    void handleKeyboard();
    void handleMouse(int16_t x, int16_t y, MouseButton button);
    MouseButton getLastKnownButtonState();
    void moveMouse(int32_t x, int32_t y, int32_t relX, int32_t relY);
    // Inputs the mouse wheel delta.
    void mouseWheel(int wheel);
    // Processes the mouse wheel delta.
    void processMouseWheel();
    void sub_407218();
    void sub_407231();
    Ui::Point getNextDragOffset();
    void processMouseOver(int16_t x, int16_t y);
    void processKeyboardInput();

    void windowPositionBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widgetIndex);

    Ui::Point getScrollLastLocation();
    Ui::Point getDragLastLocation();
    Ui::Point getTooltipMouseLocation();
    void setTooltipMouseLocation(const Ui::Point& loc);
    uint16_t getTooltipTimeout();
    void setTooltipTimeout(uint16_t tooltipTimeout);

    uint16_t getClickRepeatTicks();
    void setClickRepeatTicks(uint16_t ticks);

    bool isRightMouseButtonDown();
    void setRightMouseButtonDown(bool status);

    struct QueuedMouseInput
    {
        Ui::Point32 pos;
        uint32_t button;
    };
    void enqueueMouseButton(const QueuedMouseInput& input);
    MouseButton nextMouseInput(uint32_t& x, int16_t& y);
}
