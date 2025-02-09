#pragma once

#include "Types.hpp"
#include "Widget.h"
#include "Window.h"
#include <cstdint>

namespace OpenLoco::ToolManager
{
    using namespace Ui;

    enum class ToolEventType : uint8_t
    {
        onMouseMove,
        onMouseDown,
        onMouseDrag,
        onMouseDragEnd,
        onAbort,
        onShiftChanged,
        onControlChanged,
        onScrollNoModifier,
        onScrollShiftModifier,
        onScrollControlModifier,
        getCursor,
        count
    };

    struct ToolState
    {
        ToolEventType event;
        Point pos;
        int16_t scrollWheelChanged;
        bool shiftPressed;
        bool controlPressed;
        CursorId cursor;
    };

    using ToolCallback_t = auto (*)(Window&, const ToolState&) -> void;
    using ToolCursor_t = auto (*)(Window&, const ToolState&, bool&) -> CursorId;

    struct ToolEventList
    {
        ToolCallback_t onMouseMove = nullptr;
        ToolCallback_t onMouseDown = nullptr;
        ToolCallback_t onMouseDrag = nullptr;
        ToolCallback_t onMouseDragEnd = nullptr;
        ToolCallback_t onAbort = nullptr;
        ToolCallback_t onShiftChanged = nullptr;
        ToolCallback_t onControlChanged = nullptr;
        ToolCallback_t onScrollNoModifier = nullptr;
        ToolCallback_t onScrollShiftModifier = nullptr;
        ToolCallback_t onScrollControlModifier = nullptr;
        ToolCursor_t getCursor = nullptr;

        constexpr ToolCallback_t getToolEvent(ToolEventType event) const;
    };

    struct ToolConfiguration
    {
        ToolEventList events;
        CursorId cursor;
        WindowNumber_t windowNumber;
        WindowType type = WindowType::undefined;
    };

    Ui::Window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType);

    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t);
    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t, int16_t);
    bool isToolActive(Ui::WindowNumber_t number, const ToolConfiguration& config);
    bool toolSet(Ui::Window* w, int16_t widgetIndex, Ui::CursorId cursorId);
    bool toolSet(Ui::Window* w, ToolConfiguration config);
    void toolCancel();
    void toolCancel(Ui::WindowType, Ui::WindowNumber_t);

    // I'm sure there's a more cpp way to do this but I don't know it
    constexpr bool operator==(const ToolEventList& a, const ToolEventList& b) noexcept
    {
        for (int i = 0; i < enumValue(ToolEventType::count); i++)
        {
            if (a.getToolEvent(ToolEventType(i)) != b.getToolEvent(ToolEventType(i)))
            {
                return false;
            }
        }
        return true;
    }

    // I'm sure there's a more cpp way to do this but I don't know it
    constexpr bool operator==(const ToolConfiguration& a, const ToolConfiguration& b) noexcept
    {
        return a.events == b.events && a.cursor == b.cursor && a.type == b.type;
    }

    //  0x00523390
    Ui::WindowNumber_t getToolWindowNumber();
    void setToolWindowNumber(Ui::WindowNumber_t toolWindowNumber);

    // 0x00523392
    Ui::WindowType getToolWindowType();
    void setToolWindowType(Ui::WindowType toolWindowType);

    // 0x00523393
    Ui::CursorId getToolCursor();
    void setToolCursor(Ui::CursorId toolWindowCursor);

    // 0x00523394
    int16_t getToolWidgetIndex();
    void setToolWidgetIndex(uint16_t toolWidgetIndex);

    /*
     * gets the current cursor for the active tool
     */
    CursorId getCursor(int16_t x, int16_t y, bool& out);

    /*
     * fires the selected type and returns if the input was sunk. x, y: mouse position, z: scroll wheel input
     */
    bool fireEvent(ToolEventType event, int16_t x, int16_t y, int16_t z);
}
