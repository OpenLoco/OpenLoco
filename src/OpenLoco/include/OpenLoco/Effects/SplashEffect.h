#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct Splash : EffectEntity
    {
        uint16_t frame; // 0x28

        void tick();

        static Splash* create(const World::Pos3& pos);
    };
    static_assert(sizeof(Splash) <= sizeof(Entity));
}
