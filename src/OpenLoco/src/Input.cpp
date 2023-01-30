#include "Input.h"
#include "Audio/Audio.h"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Window.h"
#include <OpenLoco/Interop/Interop.hpp>

#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Input
{
    loco_global<uint32_t, 0x00523368> _flags;
    static loco_global<State, 0x0052336D> _state;
    static Ui::Point32 _cursorDragStart;
    loco_global<uint32_t, 0x00525374> _cursorDragState;

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
        if (_cursorDragState == 0)
        {
            _cursorDragState = 1;
            auto cursor = Ui::getCursorPos();
            _cursorDragStart = cursor;
            Ui::hideCursor();
        }
    }

    void sub_407231()
    {
        if (_cursorDragState != 0)
        {
            _cursorDragState = 0;
            Ui::setCursorPos(_cursorDragStart.x, _cursorDragStart.y);
            Ui::showCursor();
        }
    }

    Ui::Point getNextDragOffset()
    {
        auto current = Ui::getCursorPos();

        auto delta = current - _cursorDragStart;

        Ui::setCursorPos(_cursorDragStart.x, _cursorDragStart.y);

        return { static_cast<int16_t>(delta.x), static_cast<int16_t>(delta.y) };
    }
}
