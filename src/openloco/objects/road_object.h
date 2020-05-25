#pragma once

#include "../types.hpp"

namespace openloco
{
    namespace flags_12
    {
        constexpr uint8_t unk_01 = 1 << 1;
        constexpr uint8_t unk_03 = 1 << 3;
    }
#pragma pack(push, 1)
    struct road_object
    {
        string_id name;
        uint16_t road_pieces;        // 0x02
        uint16_t build_cost_factor;  // 0x04
        uint16_t sell_cost_factor;   // 0x06
        uint16_t tunnel_cost_factor; // 0x08
        uint8_t cost_index;          // 0x0A
        uint8_t pad_0B[0x0E - 0x0B];
        uint32_t var_0E;
        uint16_t flags;       // 0x12
        uint8_t num_bridges;  // 0x14
        uint8_t bridges[7];   // 0x15
        uint8_t num_stations; // 0x1C
        uint8_t stations[7];  // 0x1D
        uint8_t var_24;
        uint8_t num_mods;       // 0x25
        uint8_t mods[2];        // 0x27
        uint8_t num_compatible; // 0x28
        uint8_t pad_29[0x30 - 0x29];
    };
#pragma pack(pop)
}
