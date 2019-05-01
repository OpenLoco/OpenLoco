#include "multiplayer.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::multiplayer
{
    static loco_global<uint16_t, 0x00508F10> _flags;

    bool has_flag(flags flag)
    {
        return (_flags & (1 << flag)) != 0;
    }

    bool set_flag(flags flag)
    {
        bool val = (_flags & (1 << flag)) != 0;

        *_flags |= ~(1 << flag);
        return val;
    }

    bool reset_flag(flags flag)
    {
        bool val = (_flags & (1 << flag)) != 0;

        *_flags &= ~(1 << flag);
        return val;
    }
}
