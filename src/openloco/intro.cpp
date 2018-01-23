#include "intro.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::intro
{
    loco_global<uint8_t, 0x0050C195> _state;

    bool is_active()
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
