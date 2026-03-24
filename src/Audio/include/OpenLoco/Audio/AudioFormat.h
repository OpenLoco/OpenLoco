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

#pragma pack(push, 1)
    struct WAVEFORMATEX
    {
        int16_t wFormatTag;
        int16_t nChannels;
        int32_t nSamplesPerSec;
        int32_t nAvgBytesPerSec;
        int16_t nBlockAlign;
        int16_t wBitsPerSample;
        int16_t cbSize;
    };
#pragma pack(pop)
}
