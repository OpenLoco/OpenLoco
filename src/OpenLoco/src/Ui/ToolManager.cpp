#include "./ToolManager.h"
#include "./GameState.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::ToolManager
{
    static loco_global<Ui::WindowNumber_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<Ui::CursorId, 0x00523393> _toolWindowCursor;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIndex;

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
    Ui::CursorId getToolWindowCursor()
    {
        return _toolWindowCursor;
    }
    void setToolWindowCursor(Ui::CursorId toolWindowCursor)
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
