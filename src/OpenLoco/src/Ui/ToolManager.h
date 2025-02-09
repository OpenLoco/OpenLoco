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
        onStart,
        onStop,
        onMouseMove,
        onMouseDown,
        onMouseDrag,
        onMouseDragEnd,
        onShiftChanged,
        onControlChanged,
        onScrollNoModifier,
        onScrollShiftModifier,
        onScrollControlModifier,
        onScrollControlShiftModifier,
        count
    };

    enum class ToolFlags : uint16_t
    {
        none = 0,
        keepFlag6 = (1U << 0),
        gridlines = (1U << 1),
        notATool = (1U << 2),
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ToolFlags);

    class ToolBase
    {
    public:
        ToolFlags toolFlags;
        CursorId cursor = CursorId::pointer;
        WindowNumber_t window;
        WindowType type = WindowType::undefined;
        StringId toolTip;
        BitSet<enumValue(ToolEventType::count)> events;
        // widget is necessary for vanilla interop. Might not be necessary afterwards
        WidgetIndex_t widget;
        struct
        {
            Point pos;
            int16_t mouseWheel;
            bool dragging;
            void set(int16_t x, int16_t y, int16_t z = std::numeric_limits<int16_t>::min())
            {
                if (x != std::numeric_limits<int16_t>::min() && y != std::numeric_limits<int16_t>::min())
                {
                    pos = { x, y };
                }
                if (z != std::numeric_limits<int16_t>::min())
                {
                    mouseWheel = z;
                }
                else
                {
                    mouseWheel = 0;
                }
            }
        } input;

        bool hasEvent(ToolEventType event)
        {
            if (event >= ToolEventType::count)
            {
                return false;
            }
            return events.get(enumValue(event));
        };

        bool fireEvent(ToolEventType event, int16_t x, int16_t y, int16_t mouseWheel);

        CursorId getCursor(Window& self, bool& out, int16_t x = std::numeric_limits<int16_t>::min(), int16_t y = std::numeric_limits<int16_t>::min());
        bool activate(Window& w, bool force = false);
        void cancel();

    private:
        virtual void onStart(Window&, ToolEventType) {};
        virtual void onStop(Window&, ToolEventType) {};
        virtual void onMouseMove(Window&, ToolEventType) {};
        virtual void onMouseDown(Window&, ToolEventType) {};
        virtual void onMouseDrag(Window&, ToolEventType) {};
        virtual void onMouseDragEnd(Window&, ToolEventType) {};
        virtual void onModifierChanged(Window&, ToolEventType) {};
        virtual void onControlChanged(Window&, ToolEventType) {};
        virtual void onScroll(Window&, ToolEventType) {};
        virtual CursorId getCursorCallback(Window&, bool&) { return {}; };
    };

    Ui::Window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType);

    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t);
    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t, int16_t);
    bool isToolActive(Ui::WindowNumber_t number, const ToolBase& tool);
    bool toolSet(Ui::Window* w, int16_t widgetIndex, Ui::CursorId cursorId);
    void toolCancel();
    void toolCancel(Ui::WindowType, Ui::WindowNumber_t);

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
    void setToolWidgetIndex(int16_t widget);

    /*
     * gets the current cursor for the active tool
     */
    CursorId getCursor(bool& out, int16_t x = std::numeric_limits<int16_t>::min(), int16_t y = std::numeric_limits<int16_t>::min());

    /*
     * fires the selected type and returns if the input was sunk. x, y: mouse position, mouseWheel: mouse wheel input
     */
    bool fireEvent(ToolEventType event, int16_t x = std::numeric_limits<int16_t>::min(), int16_t y = std::numeric_limits<int16_t>::min(), int16_t mouseWheel = std::numeric_limits<int16_t>::min());
}
