#include "ToolManager.h"
#include "GameState.h"
#include "Input.h"
#include "Map/MapSelection.h"
#include "Ui/ToolManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::World;

namespace OpenLoco::ToolManager
{
    static loco_global<Ui::WindowNumber_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<Ui::CursorId, 0x00523393> _toolWindowCursor;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIndex;

    static ToolBase noTool = ToolBase();
    static ToolBase& currentTool = noTool;

    static void setCurrentTool(Window& window, ToolBase& tool)
    {
        currentTool = tool;
        currentTool.window = window.number;
    }

    static void resetCurrentTool()
    {
        currentTool = noTool;
    }

    static void setInteropVariables(const ToolBase& tool)
    {
        _toolWindowNumber = tool.window;
        _toolWindowType = tool.type;
        _toolWindowCursor = tool.cursor;
        _toolWidgetIndex = tool.widget;
    }

    Window* toolGetActiveWindow()
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return nullptr;
        }

        return WindowManager::find(currentTool.type, currentTool.window);
    }

    bool isToolActive(Ui::WindowType type)
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return false;
        }

        return currentTool.type == type;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!isToolActive(type))
        {
            return false;
        }

        return currentTool.window == number;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number, int16_t widgetIndex)
    {
        if (!isToolActive(type, number))
        {
            return false;
        }
        return getToolWidgetIndex() == widgetIndex;
    }

    bool isToolActive(Ui::WindowNumber_t number, const ToolBase& tool)
    {
        return number == currentTool.window && typeid(tool) == typeid(currentTool);
    }

    // 0x004CE367
    // tool (al)
    // widgetIndex (dx)
    // w (esi)
    bool toolSet(Ui::Window* w, int16_t widgetIndex, CursorId cursorId)
    {
        if (Input::hasFlag(Input::Flags::toolActive))
        {
            if (w->type == _toolWindowType && w->number == _toolWindowNumber && widgetIndex == _toolWidgetIndex)
            {
                toolCancel();
                return false;
            }
            else
            {
                toolCancel();
            }
        }
        currentTool = {};
        currentTool.type = w->type;
        currentTool.widget = widgetIndex;
        currentTool.window = w->number;
        currentTool.cursor = cursorId;
        Input::setFlag(Input::Flags::toolActive);
        Input::resetFlag(Input::Flags::flag6);
        ToolManager::setToolCursor(cursorId);
        ToolManager::setToolWindowType(w->type);
        ToolManager::setToolWindowNumber(w->number);
        ToolManager::setToolWidgetIndex(widgetIndex);
        return true;
    }

    // 0x004CE3D6
    void toolCancel()
    {
        if (Input::hasFlag(Input::Flags::toolActive))
        {
            Input::resetFlag(Input::Flags::toolActive);

            World::mapInvalidateSelectionRect();
            World::mapInvalidateMapSelectionTiles();

            resetMapSelectionFlag(MapSelectionFlags::enable | MapSelectionFlags::enableConstruct | MapSelectionFlags::enableConstructionArrow | MapSelectionFlags::unk_03 | MapSelectionFlags::unk_04);

            if (ToolManager::getToolWidgetIndex() >= 0)
            {
                // Invalidate tool widget
                Ui::WindowManager::invalidateWidget(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber(), ToolManager::getToolWidgetIndex());
                // Abort tool event
                Window* w = Ui::WindowManager::find(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber());
                if (w != nullptr)
                {
                    w->callToolAbort(ToolManager::getToolWidgetIndex());
                }
                currentTool = {};
            }
        }
    }

    void toolCancel(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!isToolActive(type, number))
        {
            return;
        }

        toolCancel();
    }

    Ui::WindowNumber_t getToolWindowNumber()
    {
        return _toolWindowNumber;
    }
    void setToolWindowNumber(Ui::WindowNumber_t toolWindowNumber)
    {
        _toolWindowNumber = toolWindowNumber;
    }

    Ui::WindowType getToolWindowType()
    {
        return _toolWindowType;
    }
    void setToolWindowType(Ui::WindowType toolWindowType)
    {
        _toolWindowType = toolWindowType;
    }

    Ui::CursorId getToolCursor()
    {
        return _toolWindowCursor;
    }

    void setToolCursor(Ui::CursorId toolWindowCursor)
    {
        _toolWindowCursor = toolWindowCursor;
    }

    int16_t getToolWidgetIndex()
    {
        return _toolWidgetIndex;
    }

    void setToolWidgetIndex(int16_t widget)
    {
        _toolWidgetIndex = widget;
    }


    CursorId getCursor(bool& out, int16_t x, int16_t y)
    {
        auto w = WindowManager::find(currentTool.type, currentTool.window);
        if (w == nullptr)
        {
            return currentTool.cursor;
        }
        return currentTool.getCursor(*w, out, x, y);
    }

    bool fireEvent(ToolEventType event, int16_t x, int16_t y, int16_t mouseWheel)
    {
        auto w = WindowManager::find(currentTool.type, currentTool.window);
        if (w == nullptr)
        {
            return false;
        }
        return currentTool.fireEvent(event, x, y, mouseWheel);
    }

    bool ToolBase::fireEvent(ToolEventType event, int16_t x, int16_t y, int16_t mouseWheel)
    {
        if (!Input::hasFlag(Input::Flags::toolActive) && event != ToolEventType::onStop)
        {
            return false;
        }

        input.set(x, y, mouseWheel);

        switch (event)
        {
            case ToolEventType::onMouseDrag:
                input.dragging = true;
                break;
            case ToolEventType::onMouseDragEnd:
            case ToolEventType::onStart:
                input.dragging = false;
                break;
            default:
                break;
        }

        if (!hasEvent(event))
        {
            return false;
        }

        auto w = WindowManager::find(type, window);
        if (w == nullptr)
        {
            return false;
        }
        switch (event)
        {
            case ToolEventType::onStart:
                onStart(*w, event);
                break;
            case ToolEventType::onStop:
                onStop(*w, event);
                break;
            case ToolEventType::onMouseMove:
                onMouseMove(*w, event);
                break;
            case ToolEventType::onMouseDown:
                onMouseDown(*w, event);
                break;
            case ToolEventType::onMouseDrag:
                onMouseDrag(*w, event);
                break;
            case ToolEventType::onMouseDragEnd:
                onMouseDragEnd(*w, event);
                break;
            case ToolEventType::onShiftChanged:
            case ToolEventType::onControlChanged:
                onModifierChanged(*w, event);
                break;
            case ToolEventType::onScrollControlModifier:
            case ToolEventType::onScrollControlShiftModifier:
            case ToolEventType::onScrollNoModifier:
            case ToolEventType::onScrollShiftModifier:
                onScroll(*w, event);
                break;
            default:
                break;
        }
        return true;
    }

    bool ToolBase::activate(Window& w, bool force)
    {

        if (Input::hasFlag(Input::Flags::toolActive))
        {
            if (!force && isToolActive(w.number, *this))
            {
                toolCancel();
                return false;
            }
            toolCancel();
        }
        Input::setFlag(Input::Flags::toolActive);
        Input::resetFlag(Input::Flags::flag6);

        if (toolFlags && ToolFlags::keepFlag6)
        {
            Input::setFlag(Input::Flags::flag6);
        }

        input = {};

        setCurrentTool(w, *this);
        setInteropVariables(*this);

        if (widget >= 0)
        {
            w.activatedWidgets |= (1ULL << widget);
            Ui::WindowManager::invalidateWidget(type, window, widget);
        }
        else
        {
            // TODO: for breakpoint testing. Remove before merge
            (void)input;
        }
        if (hasEvent(ToolEventType::onStart))
        {
            onStart(w, ToolEventType::onStart);
        }
        return true;
    }

    CursorId ToolBase::getCursor(Window& w, bool& out, int16_t x, int16_t y)
    {
        input.set(x, y);
        return getCursorCallback(w, out);
    }

    void ToolBase::cancel()
    {
        if (!isToolActive(window, *this))
        {
            return;
        }
        World::mapInvalidateSelectionRect();
        World::mapInvalidateMapSelectionTiles();

        resetMapSelectionFlag(MapSelectionFlags::enable | MapSelectionFlags::enableConstruct | MapSelectionFlags::enableConstructionArrow | MapSelectionFlags::unk_03 | MapSelectionFlags::unk_04);

        resetCurrentTool();

        if (toolFlags && ToolFlags::gridlines)
        {
            Ui::Windows::Main::hideGridlines();
        }

        input = {};

        auto w = WindowManager::find(type, window);
        if (w == nullptr)
        {
            window = 0;
            return;
        }
        if (window && type != WindowType::undefined && widget >= 0)
        {
            w->activatedWidgets &= ~(1U << widget);
            Ui::WindowManager::invalidateWidget(type, window, widget);
        }
        if (hasEvent(ToolEventType::onStop))
        {
            onStop(*w, ToolEventType::onStop);
        }
        window = 0;
    }
}
