#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct hill_shapes_object
    {
        string_id name;
        uint8_t hillHeightMapCount;     // 0x02
        uint8_t mountainHeightMapCount; // 0x03
        uint32_t image;                 // 0x04
        uint8_t pad_08[0x0E - 0x08];
    };
#pragma pack(pop)
}
