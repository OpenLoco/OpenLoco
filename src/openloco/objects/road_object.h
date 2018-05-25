#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct road_object
    {
        string_id name;
        uint8_t pad_02[0x12 - 0x02];
        uint16_t var_12;
        uint8_t pad_14[0x30 - 0x14];
    };
#pragma pack(pop)
}
