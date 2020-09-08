#include "Input.h"
#include "Audio/Audio.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Window.h"
#include "ui/WindowManager.h"
#include "ui/scrollview.h"

#include <map>

using namespace openloco::interop;

namespace openloco::input
{
    loco_global<uint32_t, 0x00523368> _flags;
    static loco_global<uint8_t, 0x0052336D> _state;
    static int32_t _cursor_drag_start_x;
    static int32_t _cursor_drag_start_y;
    loco_global<uint32_t, 0x00525374> _cursor_drag_state;

    void init()
    {
        _flags = 0;
        _state = 0;
    }

    bool hasFlag(input_flags value)
    {
        return (_flags & (uint32_t)value) != 0;
    }

    void setFlag(input_flags value)
    {
        _flags |= (uint32_t)value;
    }

    void resetFlag(input_flags value)
    {
        _flags &= ~(uint32_t)value;
    }

    input_state state()
    {
        return (input_state)*_state;
    }

    void state(input_state state)
    {
        _state = (uint8_t)state;
    }

    // 0x00406FEC
    void enqueueMouseButton(int32_t button)
    {
        ((void (*)(int))0x00406FEC)(button);
    }

    void sub_407218()
    {
        if (_cursor_drag_state == 0)
        {
            _cursor_drag_state = 1;
            ui::getCursorPos(_cursor_drag_start_x, _cursor_drag_start_y);
            ui::hideCursor();
        }
    }

    void sub_407231()
    {
        if (_cursor_drag_state != 0)
        {
            _cursor_drag_state = 0;
            ui::setCursorPos(_cursor_drag_start_x, _cursor_drag_start_y);
            ui::showCursor();
        }
    }
}
