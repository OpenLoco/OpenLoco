#include "Intro.h"
#include "Interop/Interop.hpp"

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

    // 0x0046AE0C
    void update()
    {
        call(0x0046AE0C);
    }
}
