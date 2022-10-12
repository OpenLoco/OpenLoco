#include "Intro.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Intro
{
    loco_global<State, 0x0050C195> _state;

    bool isActive()
    {
        return state() != State::none;
    }

    State state()
    {
        return *_state;
    }

    void state(State state)
    {
        _state = state;
    }

    // 0x0046AE0C
    void update()
    {
        call(0x0046AE0C);
    }
}
