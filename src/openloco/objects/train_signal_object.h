#pragma once

#include "../types.hpp"

namespace openloco
{
#pragma pack(push, 1)
    struct train_signal_object
    {
        string_id name;
        uint16_t track_side; // 0x02
        uint8_t var_04;
        uint8_t num_frames;        // 0x05
        uint16_t cost_factor;      // 0x06
        uint16_t sell_cost_factor; // 0x08
        uint8_t cost_index;        // 0x0A
        uint8_t var_0B;
        uint16_t var_0C;
        uint32_t var_0E;
        uint8_t var_12;
        uint8_t var_13;
        uint8_t pad_14[0x1A - 0x14];
        uint16_t designed_year; // 0x1A
        uint16_t obsolete_year; // 0x1C
    };
#pragma pack(pop)
}
