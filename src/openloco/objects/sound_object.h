#pragma once

#include <cstdint>

namespace openloco
{
#pragma pack(push, 1)
    struct sound_object
    {
        uint8_t pad_00[0x8 - 0x0];
        uint32_t volume; // 0x08
    };
#pragma pack(pop)
}
