#pragma once

#include "Types.hpp"
#include "Widget.h"
#include "Window.h"
#include <cstdint>

namespace OpenLoco::ToolManager
{
    using namespace Ui;

    using ToolEventType_t = uint8_t;
    namespace ToolEventType
    {
        constexpr ToolEventType_t onActivate = 0;
        constexpr ToolEventType_t onCancel = 1;
        constexpr ToolEventType_t onMouseMove = 2;
        constexpr ToolEventType_t onMouseDown = 3;
        constexpr ToolEventType_t onMouseDrag = 4;
        constexpr ToolEventType_t onMouseDragEnd = 5;
        constexpr ToolEventType_t onShiftChanged = 6;
        constexpr ToolEventType_t onControlChanged = 7;
        constexpr ToolEventType_t onScroll = 8;
        constexpr ToolEventType_t onScrollShiftModifier = 9;
        constexpr ToolEventType_t onScrollControlModifier = 10;
        constexpr ToolEventType_t onScrollAltModifier = 11;
        constexpr ToolEventType_t count = 12;
    };

    enum class ToolFlag : uint16_t
    {
        none = 0,
        keepFlag6 = (1U << 0),
        gridlines = (1U << 1),
        gridlinesPersistWithWindow = (1U << 2),
        dragStartsOnMouseDown = (1U << 3),
        toolPersistsThroughWindowClose = (1U << 4),
        closeWindowWithTool = (1U << 5),
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ToolFlag);

    class ToolBase
    {
    public:
        ToolFlag toolFlags;
        CursorId cursor = CursorId::pointer;
        WindowNumber_t window = std::numeric_limits<uint16_t>::max();
        WindowType type = WindowType::undefined;
        StringId toolTip;
        BitSet<ToolEventType::count> events;
        // widget is necessary for vanilla interop. Might not be necessary afterwards
        WidgetIndex_t widget = -1;
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

        bool hasEvent(ToolEventType_t event)
        {
            if (event >= ToolEventType::count)
            {
                return false;
            }
            return events.get(event);
        };
        bool hasFlag(ToolFlag flag)
        {
            return (toolFlags & flag) != ToolFlag::none;
        }
        bool fireEvent(ToolEventType_t event, int16_t x, int16_t y, int16_t mouseWheel);

        bool activate(Window& w, bool force = false);
        /*
         * if newWindow matches tool's current window, do not close window if closeToolWithWindow is set
         */
        void cancel(WindowNumber_t newWindow = 0);
        /*
         * Does not close window if closeToolWithWindow is set.
         */
        void cancelQuiet();

        bool isActive();
        bool isActive(Window& w);

        virtual void onActivate(Window&, ToolEventType_t) {};
        virtual void onCancel(Window&, ToolEventType_t) {};
        virtual void onMouseMove(Window&, ToolEventType_t) {};
        virtual void onMouseDown(Window&, ToolEventType_t) {};
        virtual void onMouseDrag(Window&, ToolEventType_t) {};
        virtual void onMouseDragEnd(Window&, ToolEventType_t) {};
        virtual void onModifierChanged(Window&, ToolEventType_t) {};
        virtual void onControlChanged(Window&, ToolEventType_t) {};
        virtual void onScroll(Window&, ToolEventType_t) {};
        virtual CursorId getCursor(Window&, CursorId current, bool&) { return current; };
        virtual ~ToolBase() {};

    private:
        void setInteropVariables();
        void trueCancel();
    };

    Ui::Window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType);

    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t);
    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t, int16_t);
    bool toolSet(Ui::Window* w, int16_t widgetIndex, Ui::CursorId cursorId);
    void toolCancel();
    void toolCancel(Ui::WindowType, Ui::WindowNumber_t);
    void toolCloseWindowAndTool(Ui::WindowType, Ui::WindowNumber_t);

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
    CursorId getCursor(CursorId current, bool& out, int16_t x = std::numeric_limits<int16_t>::min(), int16_t y = std::numeric_limits<int16_t>::min());

    /*
     * fires the selected type and returns if the input was sunk. x, y: mouse position, mouseWheel: mouse wheel input
     */
    bool fireEvent(ToolEventType_t event, int16_t mouseWheel = std::numeric_limits<int16_t>::min(), int16_t x = std::numeric_limits<int16_t>::min(), int16_t y = std::numeric_limits<int16_t>::min());
}
