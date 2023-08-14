#pragma once

#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct Fireball : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(Fireball) == 0x2A);

#pragma pack(pop)
}
