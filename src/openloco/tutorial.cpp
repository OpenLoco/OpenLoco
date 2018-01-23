#include "tutorial.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::tutorial
{
    static loco_global<uint8_t, 0x00508F19> _state;

    tutorial_state state()
    {
        return (tutorial_state)*_state;
    }

    // 0x0043C70E
    void stop()
    {
        call(0x0043C70E);
    }
}
