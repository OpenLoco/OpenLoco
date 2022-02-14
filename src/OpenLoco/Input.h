#pragma once

#include "Ui/Types.hpp"
#include "Ui/WindowManager.h"
#include "Window.h"

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

    namespace Flags
    {
        constexpr uint32_t widgetPressed = 1 << 0;
        constexpr uint32_t flag1 = 1 << 1;
        constexpr uint32_t flag2 = 1 << 2;
        constexpr uint32_t toolActive = 1 << 3;
        constexpr uint32_t flag4 = 1 << 4;
        constexpr uint32_t flag5 = 1 << 5;
        constexpr uint32_t flag6 = 1 << 6;
        constexpr uint32_t viewportScrolling = 1 << 7;
    }

    namespace MapSelectionFlags
    {
        constexpr uint8_t enable = 1 << 0;
        constexpr uint8_t enableConstruct = (1 << 1);
        constexpr uint8_t unk_02 = 1 << 2;
        constexpr uint8_t unk_03 = 1 << 3;
        constexpr uint8_t unk_04 = 1 << 4; // Vehicle orders?
        constexpr uint8_t catchmentArea = 1 << 5;
        constexpr uint8_t unk_6 = 1 << 6;
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
    bool hasFlag(uint32_t value);
    void setFlag(uint32_t value);
    void resetFlag(uint32_t value);
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

    Ui::Window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType);

    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t);
    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t, int16_t);
    bool toolSet(Ui::Window* w, int16_t widgetIndex, Ui::CursorId cursorId);
    void toolCancel();
    void toolCancel(Ui::WindowType, Ui::WindowNumber_t);
    int16_t getToolWidgetIndex();

    void enqueueText(const char* text);
    void enqueueKey(uint32_t key);
    bool hasKeyModifier(uint8_t modifier);
    uint16_t getMapSelectionFlags();
    bool hasMapSelectionFlag(uint8_t flags);
    void setMapSelectionFlags(uint8_t flags);
    void resetMapSelectionFlag(uint8_t flags);

    void handleKeyboard();
    void handleMouse(int16_t x, int16_t y, MouseButton button);
    MouseButton getLastKnownButtonState();
    void moveMouse(int32_t x, int32_t y, int32_t relX, int32_t relY);
    void sub_407218();
    void sub_407231();
    Ui::Point getNextDragOffset();
    void processMouseOver(int16_t x, int16_t y);
    void processKeyboardInput();

    void windowPositionBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widget_index);

    Ui::Point getScrollLastLocation();
    Ui::Point getDragLastLocation();
    Ui::Point getTooltipMouseLocation();
    void setTooltipMouseLocation(const Ui::Point& loc);
    uint16_t getTooltipTimeout();
    void setTooltipTimeout(uint16_t tooltipTimeout);

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
