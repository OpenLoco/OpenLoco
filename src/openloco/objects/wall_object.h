#pragma once

#include "../types.hpp"

namespace openloco
{
#pragma pack(push, 1)
    struct wall_object
    {
        string_id name;
        uint32_t sprite; // 0x02
        uint8_t var_06;
        uint8_t flags; // 0x07
        uint8_t var_08;
        uint8_t var_09;
    };
#pragma pack(pop)
}
