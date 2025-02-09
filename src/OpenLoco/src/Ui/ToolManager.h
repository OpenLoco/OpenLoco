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
        keepFlag6 = 1,
        gridlines = 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ToolFlags);

    class ToolBase
    {
    public:
        ToolFlags flags;
        CursorId cursor;
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
            void set(int16_t x, int16_t y, int16_t mouseWheel)
            {
                if (x != std::numeric_limits<int16_t>::min() && y != std::numeric_limits<int16_t>::min())
                {
                    pos = { x, y };
                }
                if (mouseWheel != std::numeric_limits<int16_t>::min())
                {
                    mouseWheel = mouseWheel;
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

        CursorId getCursor(Window& self, int16_t x, int16_t y, int16_t mouseWheel, bool& out);
        bool activate(Window& w, bool force = false);
        void cancel();

    private:
        virtual void onStart(Window& self, ToolEventType event) {};
        virtual void onStop(Window& self, ToolEventType event) {};
        virtual void onMouseMove(Window& self, ToolEventType event) {};
        virtual void onMouseDown(Window& self, ToolEventType event) {};
        virtual void onMouseDrag(Window& self, ToolEventType event) {};
        virtual void onMouseDragEnd(Window& self, ToolEventType event) {};
        virtual void onModifierChanged(Window& self, ToolEventType event) {};
        virtual void onControlChanged(Window& self, ToolEventType event) {};
        virtual void onScroll(Window& self, ToolEventType event) {};
        virtual CursorId getCursorCallback(Window& self, bool& out) { return {}; };
    };

    Ui::Window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType);

    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t);
    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t, int16_t);
    bool isToolActive(Ui::WindowNumber_t number, ToolBase& tool);
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

    /*
     * gets the current cursor for the active tool
     */
    CursorId getCursor(int16_t x, int16_t y, bool& out);

    /*
     * fires the selected type and returns if the input was sunk. x, y: mouse position, mouseWheel: mouse wheel input
     */
    bool fireEvent(ToolEventType event, int16_t x = std::numeric_limits<int16_t>::min(), int16_t y = std::numeric_limits<int16_t>::min(), int16_t mouseWheel = std::numeric_limits<int16_t>::min());
}
