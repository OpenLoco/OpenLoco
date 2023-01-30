#include "MultiPlayer.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::MultiPlayer
{
    static loco_global<uint16_t, 0x00508F10> _flags;

    bool hasFlag(flags flag)
    {
        return (_flags & (1 << flag)) != 0;
    }

    bool setFlag(flags flag)
    {
        auto result = hasFlag(flag);
        *_flags |= (1 << flag);
        return result;
    }

    bool resetFlag(flags flag)
    {
        bool val = (_flags & (1 << flag)) != 0;

        *_flags &= ~(1 << flag);
        return val;
    }
}
