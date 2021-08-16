#include "Intro.h"
#include "Gui.h"
#include "Interop/Interop.hpp"
#include "Station.h"
#include "Title.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Intro
{
    loco_global<uint8_t, 0x0050C195> _state;

    bool isActive()
    {
        return state() != State::none;
    }

    State state()
    {
        return (State)*_state;
    }

    void state(State state)
    {
        _state = (uint8_t)state;
    }

    static bool _50C196 = false;
    static int _50C190 = 0;

    // 0x0046AE0C
    void update()
    {
        addr<0x0005252E0, int32_t>() = 1;

        switch (state())
        {
            case State::none:
            case State::begin:
            case State::state_8:
            case State::state_9:
                // TODO: implement
                break;

            case State::end:
                Gfx::clear(Gfx::screenContext(), 0xA0B0C0D0);
                if (_50C196)
                {
                    _50C196 = false;
                }
                state(State::done);
                _50C190 = 0;
                break;

            case State::done:
                state(State::none);
                call(0x004523F4);
                addr<0x0005252E0, int32_t>() = 0;
                Gfx::invalidateScreen();
                initialiseViewports();
                Gui::init();
                Title::reset();
                break;
        }

        OpenLoco::sub_431695(0);
        return;
        //
        //        return;
        //        switch (state())
        //        {
        //
        //            case State::none: break;
        //            case State::begin: break;
        //            case State::state_8: break;
        //            case State::state_9: break;
        //            case State::end: break;
        //        }
        //        call(0x0046AE0C);
    }
}
