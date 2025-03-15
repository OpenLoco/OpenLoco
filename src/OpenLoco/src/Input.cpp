#include "Input.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Ui/Window.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL_mouse.h>
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

            auto result = SDL_SetRelativeMouseMode(SDL_TRUE);
            if (result == -1) // relative mode not supported
            {
                Ui::hideCursor();
            }
            else
            {
                // Consume the mouse delta x and y of SDL centering the mouse cursor
                SDL_GetRelativeMouseState(nullptr, nullptr);
            }
        }
    }

    // Cursor drag release
    void sub_407231()
    {
        if (_cursorDragState != 0)
        {
            _cursorDragState = 0;
            Ui::setCursorPos(_cursorDragStart.x, _cursorDragStart.y);
            SDL_SetRelativeMouseMode(SDL_FALSE);
            Ui::showCursor();
        }
    }

    // Returns how far the mouse cursor has moved since this function was last called,
    // and resets its position (if not using relative mouse mode).
    Ui::Point getNextDragOffset()
    {
        auto deltaX = 0;
        auto deltaY = 0;

        if (SDL_GetRelativeMouseMode())
        {
            // get the mouse deltas since the last call to SDL_GetRelativeMouseState() or since event initialization.
            SDL_GetRelativeMouseState(&deltaX, &deltaY);
        }
        else
        {
            // Fallback for if relative mode could not be enabled

            const auto oldCursorPos = Ui::getCursorPos();

            // TODO: midX, midY would ideally be the center of the screen to minimise the cursor getting stopped by the edges of the monitor, in theory
            const auto midX = _cursorDragStart.x;
            const auto midY = _cursorDragStart.y;

            deltaX = (oldCursorPos.x - midX);
            deltaY = (oldCursorPos.y - midY);

            Ui::setCursorPos(midX, midY); // Moves the mouse cursor
        }

        const auto scale = Config::get().scaleFactor;

        return { static_cast<int16_t>(deltaX / scale), static_cast<int16_t>(deltaY / scale) };
    }
}
