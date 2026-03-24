#pragma once

#include <cstdint>

namespace OpenLoco::Audio
{
    struct AudioFormat
    {
        uint32_t sampleRate;
        uint16_t channels;
        uint16_t bitsPerSample;
    };
}
