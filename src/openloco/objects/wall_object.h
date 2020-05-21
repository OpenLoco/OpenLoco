#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct wall_object
    {
        string_id name;
        uint32_t var_02;
        uint8_t var_06;
        uint8_t var_07;
        uint8_t var_08;
        uint8_t var_09;
    };
#pragma pack(pop)
}
