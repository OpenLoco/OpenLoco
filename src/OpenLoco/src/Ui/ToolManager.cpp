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
    static ToolBase* currentTool;

    static void setCurrentTool(Window& window, ToolBase* tool)
    {
        currentTool = tool;
        currentTool->window = window.number;
    }

    static void resetCurrentTool()
    {
        currentTool = &noTool;
    }

    Window* toolGetActiveWindow()
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return nullptr;
        }

        return WindowManager::find(currentTool->type, currentTool->window);
    }

    bool isToolActive(Ui::WindowType type)
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return false;
        }

        return currentTool->type == type;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!isToolActive(type))
        {
            return false;
        }

        return currentTool->window == number;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number, int16_t widgetIndex)
    {
        if (!isToolActive(type, number))
        {
            return false;
        }
        return getToolWidgetIndex() == widgetIndex;
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
        // will this leak memory?
        ToolBase tempTool{};
        tempTool.type = w->type;
        tempTool.widget = widgetIndex;
        tempTool.window = w->number;
        tempTool.cursor = cursorId;
        setCurrentTool(*w, &tempTool);
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
        if (currentTool != nullptr)
        {
            currentTool->cancel();
        }
    }

    void toolCancel(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (currentTool == nullptr)
        {
            return;
        }

        if (currentTool->type != type || currentTool->window != number)
        {
            return;
        }

        currentTool->cancel();
    }

    void toolCancelOnClose(Ui::WindowType, Ui::WindowNumber_t)
    {
        if (currentTool == nullptr)
        {
            return;
        }
        if (!currentTool->hasFlag(ToolFlag::persistThroughWindowClose))
        {
            currentTool->cancel();
        }
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

    CursorId getCursor(CursorId currentCursor, bool& out, int16_t x, int16_t y)
    {
        auto w = WindowManager::find(currentTool->type, currentTool->window);
        if (w == nullptr || currentTool->hasEvent(ToolEventType::cursorCallback))
        {
            return currentCursor;
        }
        currentTool->input.set(x, y);
        return currentTool->getCursor(*w, currentCursor, out);
    }

    bool fireEvent(ToolEventType_t event, int16_t x, int16_t y, int16_t mouseWheel)
    {
        auto w = WindowManager::find(currentTool->type, currentTool->window);
        if (w == nullptr)
        {
            return false;
        }
        return currentTool->fireEvent(event, x, y, mouseWheel);
    }

    bool ToolBase::fireEvent(ToolEventType_t event, int16_t x, int16_t y, int16_t mouseWheel)
    {
        if (!Input::hasFlag(Input::Flags::toolActive) && event != ToolEventType::onStop)
        {
            return false;
        }

        input.set(x, y, mouseWheel);

        switch (event)
        {
            case ToolEventType::onMouseDown:
                input.dragging = hasFlag(ToolFlag::dragStartsOnMouseDown);
                break;
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

    void ToolBase::setInteropVariables()
    {
        _toolWindowNumber = window;
        _toolWindowType = type;
        _toolWindowCursor = cursor;
        _toolWidgetIndex = widget;
    }

    bool ToolBase::isActive()
    {
        return this == currentTool;
    }
    bool ToolBase::isActive(Window& w)
    {
        return isActive() && w.number == window;
    }

    bool ToolBase::activate(Window& w, bool force)
    {

        if (Input::hasFlag(Input::Flags::toolActive))
        {
            if (!force && isActive(w))
            {
                toolCancel();
                return false;
            }
            toolCancel();
        }
        Input::setFlag(Input::Flags::toolActive);
        Input::resetFlag(Input::Flags::flag6);

        if (hasFlag(ToolFlag::keepFlag6))
        {
            Input::setFlag(Input::Flags::flag6);
        }

        input = {};

        setCurrentTool(w, this);
        setInteropVariables();

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

    void ToolBase::cancel()
    {
        if (!isActive()) // || !Input::hasFlag(Input::Flags::toolActive))
        {
            return;
        }
        Input::resetFlag(Input::Flags::toolActive);

        World::mapInvalidateSelectionRect();
        World::mapInvalidateMapSelectionTiles();

        resetMapSelectionFlag(MapSelectionFlags::enable | MapSelectionFlags::enableConstruct | MapSelectionFlags::enableConstructionArrow | MapSelectionFlags::unk_03 | MapSelectionFlags::unk_04);

        resetCurrentTool();

        if (toolFlags && ToolFlag::automaticGridlines)
        {
            Ui::Windows::Main::hideGridlines();
        }

        input = {};

        auto w = WindowManager::find(type, window);
        window = 0;
        if (w == nullptr)
        {
            window = 0;
            return;
        }
        if (window && type != WindowType::undefined && widget >= 0)
        {
            w->activatedWidgets &= ~(1U << widget);
            Ui::WindowManager::invalidateWidget(type, w->number, widget);
        }
        if (hasEvent(ToolEventType::onStop))
        {
            onStop(*w, ToolEventType::onStop);
        }
    }
}
