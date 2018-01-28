#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    struct industry_object
    {
        string_id name;
        uint8_t pad_02[0x11 - 0x02];
        uint8_t var_11;
        uint8_t pad_12[0xEA - 0x12];
        uint32_t var_E4;
        uint8_t pad_E8[0xEA - 0xE8];
        uint8_t var_EA;
    };
}
