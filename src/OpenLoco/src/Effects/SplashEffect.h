#pragma once

#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct Splash : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static Splash* create(const World::Pos3& pos);
    };
    static_assert(sizeof(Splash) == 0x2A);

#pragma pack(pop)
}
