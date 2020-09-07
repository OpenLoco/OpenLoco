#include "MultiPlayer.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::multiplayer
{
    static loco_global<uint16_t, 0x00508F10> _flags;

    bool hasFlag(flags flag)
    {
        return (_flags & (1 << flag)) != 0;
    }

    bool setFlag(flags flag)
    {
        bool val = (_flags & (1 << flag)) != 0;

        *_flags |= ~(1 << flag);
        return val;
    }

    bool resetFlag(flags flag)
    {
        bool val = (_flags & (1 << flag)) != 0;

        *_flags &= ~(1 << flag);
        return val;
    }
}
