#pragma once

#include "S5/S5Types.h"
#include <OpenLoco/Engine/World.hpp>
#include <cstdint>

namespace OpenLoco::World
{
    struct Animation;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Animation
    {
        uint8_t baseZ; // 0x0
        uint8_t type;  // 0x1
        S5::Pos2 pos;  // 0x2
    };
#pragma pack(pop)
    static_assert(sizeof(Animation) == 0x6);

    S5::Animation exportAnimation(const OpenLoco::World::Animation& src);
    OpenLoco::World::Animation importAnimation(const S5::Animation& src);
}
