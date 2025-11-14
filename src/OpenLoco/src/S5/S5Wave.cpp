#include "S5Wave.h"
#include "Map/Wave.h"

namespace OpenLoco::S5
{
    S5::Wave exportWave(const OpenLoco::World::Wave& src)
    {
        S5::Wave dst{};
        dst.loc = src.loc;
        dst.frame = src.frame;
        return dst;
    }

    OpenLoco::World::Wave importWave(const S5::Wave& src)
    {
        OpenLoco::World::Wave dst{};
        dst.loc = src.loc;
        dst.frame = src.frame;
        return dst;
    }
}
