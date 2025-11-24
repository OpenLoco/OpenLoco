#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct ExplosionSmoke : EffectEntity
    {
        uint16_t frame; // 0x28

        void update();

        static ExplosionSmoke* create(const World::Pos3& loc);
    };
}
