#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct road_extra_object
    {
        string_id name;
        uint16_t road_pieces;       // 0x02
        uint8_t is_overhead;        // 0x04
        uint8_t cost_index;         // 0x05
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint32_t var_0A;
        uint8_t pad_0E[0x12 - 0x0E]
    };
#pragma pack(pop)
}
