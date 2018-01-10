#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "input.h"
#include "interop/interop.hpp"

namespace openloco::input
{
    loco_global<uint32_t, 0x00523368> _flags;
    loco_global<uint8_t, 0x0052336D> _state;
    loco_global<uint32_t, 0x005251C0> _cursor_drag_start_x;
    loco_global<uint32_t, 0x005251C4> _cursor_drag_start_y;
    loco_global<uint32_t, 0x00525374> _cursor_drag_state;

    bool has_flag(input_flags value)
    {
        return (_flags & (uint32_t)value) != 0;
    }

    void set_flag(input_flags value)
    {
        _flags |= (uint32_t)value;
    }

    void reset_flag(input_flags value)
    {
        _flags &= ~(uint32_t)value;
    }

    input_state state()
    {
        return (input_state)*_state;
    }

    // 0x004BE92A
    void handle_keyboard()
    {
        LOCO_CALLPROC_X(0x004BE92A);
    }

    // 0x00406FEC
    void enqueue_mouse_button(mouse_button button)
    {
        ((void(*)(int))0x00406FEC)((int32_t)button);
    }

    void sub_407218()
    {
        if (_cursor_drag_state == 0)
        {
            _cursor_drag_state = 1;

            POINT p;
            GetCursorPos(&p);
            _cursor_drag_start_x = p.x;
            _cursor_drag_start_y = p.y;

            ShowCursor(FALSE);
        }
    }

    void sub_407231()
    {
        if (_cursor_drag_state != 0)
        {
            _cursor_drag_state = 0;
            SetCursorPos(_cursor_drag_start_x, _cursor_drag_start_y);
            ShowCursor(TRUE);
        }
    }
}
