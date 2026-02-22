#include "S5Animation.h"
#include "Map/Animation.h"

namespace OpenLoco::S5
{
    S5::Animation exportAnimation(const OpenLoco::World::Animation& src)
    {
        S5::Animation dst{};
        dst.baseZ = src.baseZ;
        dst.type = src.type;
        dst.pos = src.pos;

        return dst;
    }

    OpenLoco::World::Animation importAnimation(const S5::Animation& src)
    {
        OpenLoco::World::Animation dst{};
        dst.baseZ = src.baseZ;
        dst.type = src.type;
        dst.pos = src.pos;

        return dst;
    }
}
