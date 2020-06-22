#pragma once

#include "../types.hpp"

namespace openloco
{
    namespace train_station_flags
    {
        constexpr uint8_t recolourable = 1 << 0;
    }

#pragma pack(push, 1)
    struct train_station_object
    {
        string_id name;
        uint8_t var_02;
        uint8_t var_03;
        uint16_t track_pieces;      // 0x04
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint8_t cost_index;         // 0x0A
        uint8_t var_0B;
        uint8_t flags; // 0x0C
        uint8_t var_0D;
        uint32_t var_0E;
        uint8_t pad_12[0x22 - 0x12];
        uint8_t num_compatible; // 0x22
        uint8_t mods[7];
        uint16_t designed_year; // 0x2A
        uint16_t obsolete_year; // 0x2C
        uint8_t var_2E[16];
        uint8_t var_3E[16];
        uint8_t var_4E[16];
        uint8_t var_5E[16];
        uint8_t pad_6E[0xAC - 0x6E];
    };
#pragma pack(pop)
}
