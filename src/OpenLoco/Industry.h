#pragma once

#include "Localisation/StringManager.h"
#include "Map/Tile.h"
#include "Map/TileLoop.hpp"
#include "Objects/IndustryObject.h"
#include "Town.h"
#include "Utility/Prng.hpp"
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    using namespace map;
    using industry_id_t = uint8_t;

    namespace industry_id
    {
        constexpr industry_id_t null = std::numeric_limits<industry_id_t>::max();
    }

    namespace industry_flags
    {
        constexpr uint16_t flag_01 = 1 << 0;
        constexpr uint16_t sorted = 1 << 1;
        constexpr uint16_t closing_down = 1 << 2;
        constexpr uint16_t flag_04 = 1 << 3;
    }

#pragma pack(push, 1)
    struct industry
    {
        string_id name;
        coord_t x;                  // 0x02
        coord_t y;                  // 0x04
        uint16_t flags;             // 0x06
        Utility::prng prng;         // 0x08
        uint8_t object_id;          // 0x10
        uint8_t under_construction; // 0x11 (0xFF = Finished)
        uint8_t pad_12[0xD5 - 0x12];
        town_id_t town;           // 0xD5
        map::tile_loop tile_loop; // 0xD7
        int16_t var_DB;
        int16_t var_DD;
        uint8_t var_DF;
        uint8_t owner; // 0xE0
        uint8_t pad_E1[0x189 - 0xE1];
        uint16_t produced_cargo_quantity[2]; // 0x189
        uint8_t pad_18D[0x193 - 0x18D];
        uint16_t required_cargo_quantity[3]; // 0x193
        uint8_t pad_199[0x1A3 - 0x199];
        uint16_t produced_cargo_max[2];        // 0x1A3 (produced_cargo_quantity / 8)
        uint8_t produced_cargo_transported[2]; // 0x1A7 (%)
        uint8_t history_size[2];               // 0x1A9 (<= 20 * 12)
        uint8_t history_1[20 * 12];            // 0x1AB (20 years, 12 months)
        uint8_t history_2[20 * 12];            // 0x29B
        int32_t history_min_production[2];     // 0x38B
        uint8_t pad_393[0x453 - 0x393];

        industry_id_t id() const;
        industry_object* object() const;
        bool empty() const;
        bool canReceiveCargo() const;
        bool canProduceCargo() const;
        void getStatusString(const char* buffer);

        void update();
        void sub_45329B(const map_pos& pos);
        void sub_453354();
        void sub_454A43(map_pos pos, uint8_t bl, uint8_t bh, uint8_t dl);
    };
#pragma pack(pop)
}
