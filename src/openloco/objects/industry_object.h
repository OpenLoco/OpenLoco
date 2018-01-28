#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    struct industry_object
    {
        string_id name;
        uint8_t pad_02[0xE4 - 0x02];
        uint32_t var_E4;
        uint8_t pad_E8[0xEA - 0xE8];
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t pad_EC;
        uint8_t var_ED;
        uint8_t var_EE;
        uint8_t var_EF;
        uint8_t var_F0;
    };
}
