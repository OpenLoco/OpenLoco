#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct building_object
    {
        string_id name;
        uint8_t pad_02[0x98 - 0x02];
        uint8_t var_98;
        uint8_t pad_99[0xA0 - 0x99];
        uint8_t var_A0[2];
        uint8_t var_A2[2];
        uint8_t var_A4[2];
        uint8_t var_A6[2];
        uint8_t var_A8[2];
    };
#pragma pack(pop)
}
