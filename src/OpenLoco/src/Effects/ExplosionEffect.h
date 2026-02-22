#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct ExplosionCloud : EffectEntity
    {
        uint16_t frame; // 0x28

        void update();

        static ExplosionCloud* create(const World::Pos3& loc);
    };
    static_assert(sizeof(ExplosionCloud) <= sizeof(Entity));
}
