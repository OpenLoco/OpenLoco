#pragma once

#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct ExplosionSmoke : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static ExplosionSmoke* create(const World::Pos3& loc);
    };
    static_assert(sizeof(ExplosionSmoke) == 0x2A);

#pragma pack(pop)
}
