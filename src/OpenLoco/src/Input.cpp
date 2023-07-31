#include "Input.h"
#include "Audio/Audio.h"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Window.h"
#include <OpenLoco/Interop/Interop.hpp>
#include "Config.h"

#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Input
{
    loco_global<Flags, 0x00523368> _flags;
    static loco_global<State, 0x0052336D> _state;
    static Ui::Point32 _cursorDragStart;
    loco_global<uint32_t, 0x00525374> _cursorDragState;

    void init()
    {
        _flags = Flags::none;
        _state = State::reset;
    }

    bool hasFlag(Flags value)
    {
        return (_flags & value) != Flags::none;
    }

    void setFlag(Flags value)
    {
        _flags |= value;
    }

    void resetFlag(Flags value)
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

    // Cursor drag start
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

    // Cursor drag release
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

        auto scale = Config::get().scaleFactor;
        delta.x /= scale;
        delta.y /= scale;

        return { static_cast<int16_t>(delta.x), static_cast<int16_t>(delta.y) };
    }
}
