#pragma once

#include "../types.hpp"

namespace openloco
{
    namespace flags_22
    {
        constexpr uint8_t unk_02 = 1 << 2;
    }
#pragma pack(push, 1)
    struct track_object
    {
        string_id name;
        uint16_t track_pieces;         // 0x02
        uint16_t station_track_pieces; // 0x04
        uint8_t var_06;
        uint8_t num_compatible; // 0x07
        uint8_t num_mods;       // 0x08
        uint8_t num_signals;    // 0x09
        uint8_t mods[4];        // 0x0A
        uint16_t var_0E;
        uint8_t pad_14[0x14 - 0x10];
        uint16_t build_cost_factor;  // 0x14
        uint16_t sell_cost_factor;   // 0x16
        uint16_t tunnel_cost_factor; // 0x18
        uint8_t cost_index;          // 0x1A
        uint8_t var_1B;
        uint16_t curve_speed; // 0x1C
        uint32_t var_1E;
        uint16_t flags;         // 0x22
        uint8_t num_bridges;    // 0x24
        uint8_t bridges[7];     // 0x25
        uint8_t num_stations;   // 0x2C
        uint8_t stations[7];    // 0x2D
        uint8_t display_offset; // 0x34
    };
#pragma pack(pop)
}
