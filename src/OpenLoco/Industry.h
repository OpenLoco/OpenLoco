#pragma once

#include "Core/Span.hpp"
#include "Map/Tile.h"
#include "Map/TileLoop.hpp"
#include "Types.hpp"
#include "Utility/Prng.hpp"
#include <limits>

namespace OpenLoco
{
    struct IndustryObject;

    struct Unk4F9274
    {
        Map::Pos2 pos;
        uint8_t unk;
    };
    const stdx::span<const Unk4F9274> getUnk4F9274(bool type);

    namespace IndustryFlags
    {
        constexpr uint16_t flag_01 = 1 << 0;
        constexpr uint16_t sorted = 1 << 1;
        constexpr uint16_t closingDown = 1 << 2;
        constexpr uint16_t flag_04 = 1 << 3;
    }

#pragma pack(push, 1)
    struct Industry
    {
        string_id name;
        coord_t x;                  // 0x02
        coord_t y;                  // 0x04
        uint16_t flags;             // 0x06
        Utility::prng prng;         // 0x08
        uint8_t object_id;          // 0x10
        uint8_t under_construction; // 0x11 (0xFF = Finished)
        uint16_t pad_12;
        uint8_t numTiles;        // 0x14
        Map::Pos3 tiles[32];     // 0x15
        TownId town;             // 0xD5
        Map::TileLoop tile_loop; // 0xD7
        int16_t var_DB;
        int16_t var_DD;
        uint8_t var_DF;
        CompanyId owner;     // 0xE0
        uint32_t var_E1[32]; // 0xE1 stations bit set
        StationId producedCargoStatsStation[2][4];
        uint8_t producedCargoStatsRating[2][4];
        uint8_t pad_179[0x189 - 0x179];
        uint16_t produced_cargo_quantity[2]; // 0x189
        uint16_t var_18D[3];
        uint16_t required_cargo_quantity[3]; // 0x193
        uint16_t var_199[3];
        uint8_t pad_19F[0x1A3 - 0x19F];
        uint16_t produced_cargo_max[2];        // 0x1A3 (produced_cargo_quantity / 8)
        uint8_t produced_cargo_transported[2]; // 0x1A7 (%)
        uint8_t historySize[2];                // 0x1A9 (<= 20 * 12)
        uint8_t history_1[20 * 12];            // 0x1AB (20 years, 12 months)
        uint8_t history_2[20 * 12];            // 0x29B
        int32_t history_min_production[2];     // 0x38B
        uint8_t pad_393[0x453 - 0x393];

        IndustryId id() const;
        const IndustryObject* getObject() const;
        bool empty() const;
        bool canReceiveCargo() const;
        bool canProduceCargo() const;
        void getStatusString(const char* buffer);

        void update();
        void sub_45329B(const Map::Pos2& pos);
        void sub_453354();
        void sub_454A43(const Map::Pos2& pos, uint8_t bl, uint8_t bh, uint8_t dl);
        void createMapAnimations();
        void updateProducedCargoStats();
    };
#pragma pack(pop)

    static_assert(sizeof(Industry) == 0x453);
}
