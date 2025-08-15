#include "Input.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Localisation/StringIds.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Ui/Window.h"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <map>

#include <SDL2/SDL.h>
#pragma warning(disable : 4121) // alignment of a member was sensitive to packing
#include <SDL2/SDL_syswm.h>
#pragma warning(default : 4121) // alignment of a member was sensitive to packing

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Input
{
    static Flags _flags;
    static State _state;
    static Ui::Point32 _cursorDragStart;
    static uint32_t _cursorDragState;
    static bool _exitRequested = false;

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
        Logging::verbose("Input state change, old: {0}, new: {1}", *_state, state);
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

    // 0x004072EC
    bool processMessagesMini()
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    return false;
                case SDL_WINDOWEVENT:
                    switch (e.window.event)
                    {
                        case SDL_WINDOWEVENT_MOVED:
                            Ui::windowPositionChanged(e.window.data1, e.window.data2);
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            Ui::windowSizeChanged(e.window.data1, e.window.data2);
                            break;
                    }
                    break;
            }
        }
        return false;
    }

    // 0x0040726D
    bool processMessages()
    {
        // The game has more than one loop for processing messages, if the secondary loop receives
        // SDL_QUIT then the message would be lost for the primary loop so we have to keep track of it.
        if (_exitRequested)
        {
            return false;
        }

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    _exitRequested = true;
                    return false;
                case SDL_WINDOWEVENT:
                    switch (e.window.event)
                    {
                        case SDL_WINDOWEVENT_MOVED:
                            Ui::windowPositionChanged(e.window.data1, e.window.data2);
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            Ui::windowSizeChanged(e.window.data1, e.window.data2);
                            break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                {
                    auto scaleFactor = Config::get().scaleFactor;
                    auto x = static_cast<int32_t>(e.motion.x / scaleFactor);
                    auto y = static_cast<int32_t>(e.motion.y / scaleFactor);
                    auto xrel = static_cast<int32_t>(e.motion.xrel / scaleFactor);
                    auto yrel = static_cast<int32_t>(e.motion.yrel / scaleFactor);
                    moveMouse(x, y, xrel, yrel);
                    break;
                }
                case SDL_MOUSEWHEEL:
                    mouseWheel(e.wheel.y);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                {
                    auto scaleFactor = Config::get().scaleFactor;
                    const auto x = static_cast<int32_t>(e.button.x / scaleFactor);
                    const auto y = static_cast<int32_t>(e.button.y / scaleFactor);
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            enqueueMouseButton({ { x, y }, 1 });
                            break;
                        case SDL_BUTTON_RIGHT:
                            enqueueMouseButton({ { x, y }, 2 });
                            setRightMouseButtonDown(true);
                            break;
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    auto scaleFactor = Config::get().scaleFactor;
                    const auto x = static_cast<int32_t>(e.button.x / scaleFactor);
                    const auto y = static_cast<int32_t>(e.button.y / scaleFactor);
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            enqueueMouseButton({ { x, y }, 3 });
                            break;
                        case SDL_BUTTON_RIGHT:
                            enqueueMouseButton({ { x, y }, 4 });
                            setRightMouseButtonDown(false);
                            break;
                    }
                    break;
                }
                case SDL_KEYDOWN:
                {
                    auto keycode = e.key.keysym.sym;

#if !(defined(__APPLE__) && defined(__MACH__))
                    // Toggle fullscreen when ALT+RETURN is pressed
                    if (keycode == SDLK_RETURN)
                    {
                        if ((e.key.keysym.mod & KMOD_LALT) || (e.key.keysym.mod & KMOD_RALT))
                        {
                            Ui::toggleFullscreenDesktop();
                        }
                    }
#endif

                    handleKeyInput(keycode);
                    break;
                }
                case SDL_KEYUP:
                    break;
                case SDL_TEXTINPUT:
                    enqueueText(e.text.text);
                    break;
            }
        }

        readKeyboardState();
        return true;
    }
}
