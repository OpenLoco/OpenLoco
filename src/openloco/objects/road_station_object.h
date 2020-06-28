#pragma once

#include "../types.hpp"

namespace openloco
{
    namespace road_station_flags
    {
        constexpr uint8_t recolourable = 1 << 0;
    }

#pragma pack(push, 1)
    struct road_station_object
    {
        string_id name;
        uint8_t pad_02[0x04 - 0x02];
        uint16_t road_pieces;       // 0x04
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint8_t cost_index;         // 0x0A
        uint8_t flags;              // 0x0B
        uint32_t var_0C;
        uint8_t pad_10[0x20 - 0x10];
        uint8_t num_compatible; // 0x20
        uint8_t mods[7];        // 0x21
        uint16_t designed_year; // 0x28
        uint16_t obsolete_year; // 0x2A
        uint8_t pad_2C[0x6E - 0x2C];
    };
#pragma pack(pop)
}
