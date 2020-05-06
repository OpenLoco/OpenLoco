#pragma once

#include "localisation/stringmgr.h"
#include "map/tile.h"
#include "map/tile_loop.hpp"
#include "objects/industry_object.h"
#include "town.h"
#include "utility/prng.hpp"
#include <cstdint>
#include <limits>

namespace openloco
{
    using namespace map;
    using industry_id_t = uint16_t;

    namespace industry_id
    {
        constexpr industry_id_t null = std::numeric_limits<industry_id_t>::max();
    }

    namespace industry_flags
    {
        constexpr uint16_t flag_01 = 1 << 0;
    }

#pragma pack(push, 1)
    struct industry
    {
        string_id name;
        coord_t x;          // 0x02
        coord_t y;          // 0x04
        uint16_t flags;     // 0x06
        utility::prng prng; // 0x08
        uint8_t object_id;  // 0x10
        uint8_t var_11;
        uint8_t pad_12[0xD5 - 0x12];
        town_id_t town;           // 0xD5
        map::tile_loop tile_loop; // 0xD7
        int16_t var_DB;
        int16_t var_DD;
        uint8_t var_DF;
        uint8_t owner; // 0xE0
        uint8_t pad_E1[0x189 - 0xE1];
        uint16_t produced_cargo[2]; // 0x189
        uint8_t pad_18D[0x193 - 0x18D];
        uint16_t received_cargo[3]; // 0x193
        uint8_t pad_199[0x1A3 - 0x199];
        uint16_t var_1A3;
        uint16_t var_1A5;
        uint8_t var_1A7[2];
        uint8_t history_size[2];           // 0x1A9 (<= 20 * 12)
        uint8_t history_1[20 * 12];        // 0x1AB (20 years, 12 months)
        uint8_t history_2[20 * 12];        // 0x29B
        int32_t history_min_production[2]; // 0x38B
        uint8_t pad_393[0x453 - 0x393];

        industry_id_t id() const;
        industry_object* object() const;
        bool empty() const;

        void update();
        void sub_454A43(map_pos pos, uint8_t bl, uint8_t bh, uint8_t dl);
    };
#pragma pack(pop)
}
