#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct Smoke : EffectEntity
    {
        uint16_t frame; // 0x28

        void update();

        static Smoke* create(World::Pos3 loc);
    };
    static_assert(sizeof(Smoke) <= sizeof(Entity));
}
