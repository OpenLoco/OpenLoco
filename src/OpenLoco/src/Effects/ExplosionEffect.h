#pragma once

#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct ExplosionCloud : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static ExplosionCloud* create(const World::Pos3& loc);
    };
    static_assert(sizeof(ExplosionCloud) == 0x2A);

#pragma pack(pop)
}
