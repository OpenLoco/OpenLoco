#pragma once

#include <cstdint>

namespace OpenLoco::Audio
{
    enum class EffectType : uint8_t
    {
        reverb,
        doppler,
    };

    struct ReverbParams
    {
        float decay = 1.0f;
        float density = 1.0f;
        float diffusion = 1.0f;
        float gain = 0.32f;
    };

    struct DopplerParams
    {
        float factor = 1.0f;
        float velocity = 343.3f;
    };
}
