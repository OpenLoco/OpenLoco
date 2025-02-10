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

    static ToolBase tempTools[2] = {};

    static ToolBase noTool = ToolBase();
    static ToolBase* currentTool = &noTool;

    static void setCurrentTool(Window& window, ToolBase* tool)
    {
        currentTool = tool;
        currentTool->window = window.number;
        Input::setFlag(Input::Flags::toolActive);
    }

    static void resetCurrentTool()
    {
        Input::resetFlag(Input::Flags::toolActive);
        currentTool = &noTool;
    }

    Window* toolGetActiveWindow()
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return nullptr;
        }
        if (currentTool == nullptr)
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
        if (currentTool == nullptr)
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
        if (currentTool == nullptr)
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
        // will this leak memory?
        ToolBase* tempTool = tempTools;
        if (tempTool->isActive())
        {
            tempTool++;
        }
        tempTool->type = w->type;
        tempTool->widget = widgetIndex;
        tempTool->cursor = cursorId;
        return tempTool->activate(*w);
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
        if (currentTool == nullptr)
        {
            return currentCursor;
        }
        auto w = WindowManager::find(currentTool->type, currentTool->window);
        if (w == nullptr)
        {
            return currentCursor;
        }
        currentTool->input.set(x, y);
        return currentTool->getCursor(*w, currentCursor, out);
    }

    bool OpenLoco::ToolManager::fireEvent(ToolEventType_t event, int16_t mouseWheel, int16_t x, int16_t y)
    {
        if (currentTool == nullptr)
        {
            return false;
        }
        auto w = WindowManager::find(currentTool->type, currentTool->window);
        if (w == nullptr)
        {
            return false;
        }
        return currentTool->fireEvent(event, x, y, mouseWheel);
    }

    bool ToolBase::fireEvent(ToolEventType_t event, int16_t x, int16_t y, int16_t mouseWheel)
    {
        if (!Input::hasFlag(Input::Flags::toolActive) && event != ToolEventType::onCancel)
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
            case ToolEventType::onActivate:
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
            case ToolEventType::onActivate:
                onActivate(*w, event);
                break;
            case ToolEventType::onCancel:
                onCancel(*w, event);
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
            case ToolEventType::onScroll:
            case ToolEventType::onScrollControlModifier:
            case ToolEventType::onScrollShiftModifier:
            case ToolEventType::onScrollAltModifier:
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
                if (currentTool != nullptr)
                {
                    currentTool->cancel();
                }
                return false;
            }
            if (currentTool != nullptr)
            {
                currentTool->cancel(w.number);
            }
        }

        input = {};

        setCurrentTool(w, this);
        setInteropVariables();

        Input::resetFlag(Input::Flags::flag6);

        if (hasFlag(ToolFlag::keepFlag6))
        {
            Input::setFlag(Input::Flags::flag6);
        }
        if (!hasFlag(ToolFlag::gridlines))
        {
            Ui::Windows::Main::hideGridlines();
        }

        if (widget >= 0)
        {
            w.activatedWidgets |= (1ULL << widget);
            Ui::WindowManager::invalidateWidget(type, window, widget);
        }
        if (hasEvent(ToolEventType::onActivate))
        {
            onActivate(w, ToolEventType::onActivate);
        }
        return true;
    }

    void ToolBase::trueCancel()
    {
        if (!isActive() || !Input::hasFlag(Input::Flags::toolActive))
        {
            return;
        }

        World::mapInvalidateSelectionRect();
        World::mapInvalidateMapSelectionTiles();

        resetMapSelectionFlag(MapSelectionFlags::enable | MapSelectionFlags::enableConstruct | MapSelectionFlags::enableConstructionArrow | MapSelectionFlags::unk_03 | MapSelectionFlags::unk_04);

        resetCurrentTool();

        if (hasFlag(ToolFlag::gridlines) && !hasFlag(ToolFlag::gridlinesPersistWithWindow))
        {
            Ui::Windows::Main::hideGridlines();
        }

        input = {};

        auto w = WindowManager::find(type, window);
        if (w == nullptr)
        {
            window = std::numeric_limits<uint16_t>::max();
            return;
        }
        if (widget >= 0)
        {
            w->activatedWidgets &= ~(1U << widget);
            Ui::WindowManager::invalidateWidget(type, w->number, widget);
        }
        if (hasEvent(ToolEventType::onCancel))
        {
            onCancel(*w, ToolEventType::onCancel);
        }
        window = std::numeric_limits<uint16_t>::max();
    }

    void ToolBase::cancel(WindowNumber_t newWindow)
    {
        if (!isActive()) // || !Input::hasFlag(Input::Flags::toolActive))
        {
            return;
        }
        auto w = WindowManager::find(type, window);
        trueCancel();
        if (w != nullptr && w->number != newWindow && hasFlag(ToolFlag::closeWindowWithTool))
        {
            // currentTool is now unset, so no infinite loop
            WindowManager::close(w);
        }
    }

    void ToolBase::cancelQuiet()
    {
        trueCancel();
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

    void toolCloseWindowAndTool(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (currentTool == nullptr)
        {
            return;
        }
        if (currentTool->type != type || currentTool->window != number)
        {
            return;
        }
        if (!currentTool->hasFlag(ToolFlag::toolPersistsThroughWindowClose))
        {
            if (currentTool->hasFlag(ToolFlag::gridlinesPersistWithWindow))
            {
                Ui::Windows::Main::hideGridlines();
            }
            currentTool->cancelQuiet();
        }
    }
}
