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

    Window* toolGetActiveWindow()
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return nullptr;
        }

        return WindowManager::find(_toolWindowType, _toolWindowNumber);
    }

    bool isToolActive(Ui::WindowType type)
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
            return false;

        return _toolWindowType == type;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!isToolActive(type))
            return false;

        return _toolWindowNumber == number;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number, int16_t widgetIndex)
    {
        if (!isToolActive(type, number))
            return false;
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
            if (w->type == _toolWindowType && w->number == _toolWindowNumber &&
                widgetIndex == _toolWidgetIndex)
            {
                toolCancel();
                return false;
            }
            else
            {
                toolCancel();
            }
        }

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
                    w->callToolAbort(ToolManager::getToolWidgetIndex());
            }
        }
    }

    void toolCancel(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!isToolActive(type, number))
            return;

        toolCancel();
    }

    //  0x00523390
    Ui::WindowNumber_t getToolWindowNumber()
    {
        return _toolWindowNumber;
    }
    void setToolWindowNumber(Ui::WindowNumber_t toolWindowNumber)
    {
        _toolWindowNumber = toolWindowNumber;
    }

    // 0x00523392
    Ui::WindowType getToolWindowType()
    {
        return _toolWindowType;
    }
    void setToolWindowType(Ui::WindowType toolWindowType)
    {
        _toolWindowType = toolWindowType;
    }

    // 0x00523393
    Ui::CursorId getToolCursor()
    {
        return _toolWindowCursor;
    }
    void setToolCursor(Ui::CursorId toolWindowCursor)
    {
        _toolWindowCursor = toolWindowCursor;
    }

    // 0x00523394
    int16_t getToolWidgetIndex()
    {
        return _toolWidgetIndex;
    }
    void setToolWidgetIndex(uint16_t toolWidgetIndex)
    {
        _toolWidgetIndex = toolWidgetIndex;
    }
}
