#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct competitor_object
    {
        string_id var_00;        // 0x00
        string_id var_02;        // 0x02
        uint32_t var_04;         // 0x04
        uint32_t var_08;         // 0x08
        uint32_t emotions;       // 0x0C
        uint32_t images[9];      // 0x10
        uint8_t intelligence;    // 0x34
        uint8_t agressiveness;   // 0x35
        uint8_t competitiveness; // 0x36
        uint8_t var_37;          // 0x37
    };
    static_assert(sizeof(competitor_object) == 0x38);
#pragma pack(pop)
}
