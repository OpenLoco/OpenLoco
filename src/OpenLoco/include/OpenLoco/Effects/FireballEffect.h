#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct Fireball : EffectEntity
    {
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(Fireball) <= sizeof(Entity));
}
