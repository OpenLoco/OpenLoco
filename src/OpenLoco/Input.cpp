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
    static Ui::Point32 _cursor_drag_start;
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

    void sub_407218()
    {
        if (_cursor_drag_state == 0)
        {
            _cursor_drag_state = 1;
            auto cursor = Ui::getCursorPos();
            _cursor_drag_start = cursor;
            Ui::hideCursor();
        }
    }

    void sub_407231()
    {
        if (_cursor_drag_state != 0)
        {
            _cursor_drag_state = 0;
            Ui::setCursorPos(_cursor_drag_start.x, _cursor_drag_start.y);
            Ui::showCursor();
        }
    }

    Ui::Point getNextDragOffset()
    {
        auto current = Ui::getCursorPos();

        auto delta = current - _cursor_drag_start;

        Ui::setCursorPos(_cursor_drag_start.x, _cursor_drag_start.y);

        return { static_cast<int16_t>(delta.x), static_cast<int16_t>(delta.y) };
    }
}
