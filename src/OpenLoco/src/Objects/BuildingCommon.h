#pragma once
#include <cstdint>

namespace OpenLoco
{
#pragma pack(push, 1)
    struct BuildingPartAnimation
    {
        uint8_t numFrames;      // 0x0 Must be a power of 2 (0 = no part animation, could still have animation sequence)
        uint8_t animationSpeed; // 0x1 Also encodes in bit 7 if the animation is position modified
    };
#pragma pack(pop)
    static_assert(sizeof(BuildingPartAnimation) == 0x2);
}
