#pragma once

#include <cstdint>

namespace OpenLoco::Audio
{
    struct ReverbParams
    {
        float density = 1.0f;
        float diffusion = 1.0f;
        float gain = 0.32f;
        float gainHF = 0.89f;
        float decayTime = 1.49f;
        float decayHFRatio = 0.83f;
        float reflectionsGain = 0.05f;
        float reflectionsDelay = 0.007f;
        float lateReverbGain = 1.26f;
        float lateReverbDelay = 0.011f;
    };
}
