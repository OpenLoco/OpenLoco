#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct ExplosionSmoke : EffectEntity
    {
        uint16_t frame; // 0x28

        void tick();

        static ExplosionSmoke* create(const World::Pos3& loc);
    };
    static_assert(sizeof(ExplosionSmoke) <= sizeof(Entity));
}
