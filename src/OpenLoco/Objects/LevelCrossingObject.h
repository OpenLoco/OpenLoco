#pragma once

#include "../Localisation/StringManager.h"

namespace openloco
{
#pragma pack(push, 1)
    struct level_crossing_object
    {
        string_id name;
        int16_t costFactor;         // 0x02
        uint8_t pad_04[0x06 - 0x04]; // 0x04
        uint8_t costIndex;           // 0x06
        uint8_t var_07;
        uint8_t closingFrames; // 0x08
        uint8_t closedFrames;  // 0x09
        uint8_t pad_0A[0x0C - 0x0A];
        uint16_t designedYear; // 0x0C
        uint32_t image;        // 0x0E
    };
#pragma pack(pop)
}
