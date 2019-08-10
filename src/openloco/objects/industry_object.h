#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct industry_object
    {
        string_id name;
        uint8_t pad_02[0xDE - 0x02];
        int8_t produces[2];
        int8_t accepts[3];
        uint32_t flags;
        uint8_t pad_E8[0xEA - 0xE8];
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t pad_EC;
        uint8_t var_ED;
        uint8_t var_EE;
        uint8_t var_EF;
        uint8_t var_F0;
    };
#pragma pack(pop)
}
