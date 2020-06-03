#pragma once

#include "../types.hpp"

namespace openloco
{

#pragma pack(push, 1)
    struct track_extra_object
    {
        string_id name;
        uint16_t track_pieces;       // 0x02
        uint8_t is_overhead;        // 0x04
        uint8_t cost_index;         // 0x05
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint8_t pad_0A[0x0E - 0x0A];
        uint32_t var_0E;
    };
#pragma pack(pop)
}
