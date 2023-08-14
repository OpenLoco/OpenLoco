#pragma once

#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct Smoke : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static Smoke* create(World::Pos3 loc);
    };
    static_assert(sizeof(Smoke) == 0x2A);

#pragma pack(pop)
}
