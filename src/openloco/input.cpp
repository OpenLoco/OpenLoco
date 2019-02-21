#include "input.h"
#include "audio/audio.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "ui.h"
#include "ui/WindowManager.h"
#include "ui/scrollview.h"
#include "window.h"

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

    void state(input_state state)
    {
        _state = (uint8_t)state;
    }

    // 0x004BE92A
    void handle_keyboard()
    {
        call(0x004BE92A);
    }

    // 0x00406FEC
    void enqueue_mouse_button(mouse_button button)
    {
        ((void (*)(int))0x00406FEC)((int32_t)button);
    }

    void move_mouse(int32_t x, int32_t y, int32_t relX, int32_t relY)
    {
        addr<0x0113E72C, int32_t>() = x;
        addr<0x0113E730, int32_t>() = y;
        addr<0x0114084C, int32_t>() = relX;
        addr<0x01140840, int32_t>() = relY;
    }

    void sub_407218()
    {
        if (_cursor_drag_state == 0)
        {
            _cursor_drag_state = 1;
            ui::get_cursor_pos(_cursor_drag_start_x, _cursor_drag_start_y);
            ui::hide_cursor();
        }
    }

    void sub_407231()
    {
        if (_cursor_drag_state != 0)
        {
            _cursor_drag_state = 0;
            ui::set_cursor_pos(_cursor_drag_start_x, _cursor_drag_start_y);
            ui::show_cursor();
        }
    }
}
