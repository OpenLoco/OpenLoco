#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct Splash : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static Splash* create(const World::Pos3& pos);
    };
}
