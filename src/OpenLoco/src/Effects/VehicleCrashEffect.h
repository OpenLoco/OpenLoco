#pragma once

#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct VehicleCrashParticle : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
        uint8_t pad_2A[0x2E - 0x2A];
        ColourScheme colourScheme;  // 0x2E
        uint16_t crashedSpriteBase; // 0x30 crashed_sprite_base

        void update();
    };
    static_assert(sizeof(VehicleCrashParticle) == 0x32);

#pragma pack(pop)
}
