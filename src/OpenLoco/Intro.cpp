#include "Intro.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Intro
{
    loco_global<uint8_t, 0x0050C195> _state;

    bool isActive()
    {
        return state() != intro_state::none;
    }

    intro_state state()
    {
        return (intro_state)*_state;
    }

    void state(intro_state state)
    {
        _state = (uint8_t)state;
    }

    // 0x0046AE0C
    void update()
    {
        call(0x0046AE0C);
    }
}
