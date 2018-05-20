#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct airport_object
    {
        string_id name;
        uint8_t pad_02[0xB2 - 0x02];
        uint8_t * var_B2;
        uint8_t pad_B6[0xBA - 0xB6];
    };
#pragma pack(pop)
}
