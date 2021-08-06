#include "Input.h"
#include "Audio/Audio.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Window.h"

#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Input
{
    loco_global<uint32_t, 0x00523368> _flags;
    static loco_global<State, 0x0052336D> _state;
    static int32_t _cursor_drag_start_x;
    static int32_t _cursor_drag_start_y;
    loco_global<uint32_t, 0x00525374> _cursor_drag_state;

    void init()
    {
        _flags = 0;
        _state = State::reset;
    }

    bool hasFlag(uint32_t value)
    {
        return (_flags & value) != 0;
    }

    void setFlag(uint32_t value)
    {
        _flags |= value;
    }

    void resetFlag(uint32_t value)
    {
        _flags &= ~value;
    }

    State state()
    {
        return *_state;
    }

    void state(State state)
    {
        _state = state;
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
            auto cursor = Ui::getCursorPos();
            _cursor_drag_start_x = cursor.x;
            _cursor_drag_start_y = cursor.y;
            Ui::hideCursor();
        }
    }

    void sub_407231()
    {
        if (_cursor_drag_state != 0)
        {
            _cursor_drag_state = 0;
            Ui::setCursorPos(_cursor_drag_start_x, _cursor_drag_start_y);
            Ui::showCursor();
        }
    }

    Ui::Point getNextDragOffset()
    {
        auto current = Ui::getCursorPos();

        auto deltaX = current.x - _cursor_drag_start_x;
        auto deltaY = current.y - _cursor_drag_start_y;

        Ui::setCursorPos(_cursor_drag_start_x, _cursor_drag_start_y);

        return { static_cast<int16_t>(deltaX), static_cast<int16_t>(deltaY) };
    }
}
