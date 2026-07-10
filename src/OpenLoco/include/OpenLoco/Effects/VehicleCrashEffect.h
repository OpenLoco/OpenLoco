#pragma once

#include "Effect.h"

namespace OpenLoco
{
    struct VehicleCrashParticle : EffectEntity
    {
        uint16_t timeToLive;        // 0x26
        uint16_t frame;             // 0x28
        ColourScheme colourScheme;  // 0x2E
        uint16_t crashedSpriteBase; // 0x30 crashed_sprite_base
        World::Pos3 velocity;       // 0x32
        // TODO: Convert this to World::Pos3 once we can change the coord type to int32_t.
        int32_t accelerationX; // 0x38
        int32_t accelerationY; // 0x3C
        int32_t accelerationZ; // 0x40

        void update();

        static VehicleCrashParticle* create(const World::Pos3& loc, const ColourScheme colourScheme);
    };
    static_assert(sizeof(VehicleCrashParticle) <= sizeof(Entity));
}
