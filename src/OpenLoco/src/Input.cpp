#include "Input.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Ui/Window.h"
#include <map>

namespace OpenLoco::Input
{
    static Flags _flags;
    static State _state;
    static Ui::Point32 _cursorDragStart;
    static uint32_t _cursorDragState;

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
        return _state;
    }

    void state(State state)
    {
        _state = state;
    }

    // 0x00407218
    void startCursorDrag()
    {
        if (_cursorDragState == 0)
        {
            _cursorDragState = 1;
            auto cursor = Ui::getCursorPos();
            _cursorDragStart = cursor;
            Ui::hideCursor();
        }
    }

    // 0x00407231
    void stopCursorDrag()
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
