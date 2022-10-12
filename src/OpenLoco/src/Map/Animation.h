#pragma once

#include "Map.hpp"

namespace OpenLoco::Map
{
#pragma pack(push, 1)
    struct Animation
    {
        uint8_t baseZ;
        uint8_t type;
        Map::Pos2 pos;
    };
    static_assert(sizeof(Animation) == 6);
#pragma pack(pop)
}
