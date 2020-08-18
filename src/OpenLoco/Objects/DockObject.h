#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct dock_object
    {
        string_id name;
        uint16_t build_cost_factor; // 0x02
        uint16_t sell_cost_factor;  // 0x04
        uint8_t cost_index;         // 0x06
        uint8_t var_07;
        uint32_t image;
        uint8_t pad_0C[0x12 - 0x0C];
        uint8_t num_aux_01;     // 0x12
        uint8_t num_aux_02_ent; // 0x13
        uint8_t pad_14[0x20 - 0x14];
        uint16_t designed_year; // 0x20
        uint16_t obsolete_year; // 0x22
        uint8_t pad_24[0x28 - 0x24];
    };
#pragma pack(pop)
}
