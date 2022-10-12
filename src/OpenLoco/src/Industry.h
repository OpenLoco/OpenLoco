#pragma once

#include "Core/BitSet.hpp"
#include "Core/Span.hpp"
#include "Limits.h"
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
    const stdx::span<const Unk4F9274> getBuildingTileOffsets(bool type);

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
        Utility::Prng prng;         // 0x08
        uint8_t objectId;           // 0x10
        uint8_t under_construction; // 0x11 (0xFF = Finished)
        uint16_t pad_12;
        uint8_t numTiles;       // 0x14
        Map::Pos3 tiles[32];    // 0x15
        TownId town;            // 0xD5
        Map::TileLoop tileLoop; // 0xD7
        int16_t var_DB;
        int16_t var_DD;
        uint8_t var_DF;
        CompanyId owner;                           // 0xE0
        BitSet<Limits::kMaxStations> var_E1;       // 0xE1 stations bit set
        StationId producedCargoStatsStation[2][4]; // 0x161
        uint8_t producedCargoStatsRating[2][4];    // 0x171
        uint16_t productionRate[2];                // 0x179
        uint16_t var_17D[2];
        uint16_t var_181[2];
        uint16_t producedCargoQuantityMonthlyTotal[2];           // 0x185
        uint16_t producedCargoQuantityPreviousMonth[2];          // 0x189
        uint16_t receivedCargoQuantityMonthlyTotal[3];           // 0x18D
        uint16_t receivedCargoQuantityPreviousMonth[3];          // 0x193
        uint16_t receivedCargoQuantityDailyTotal[3];             // 0x199
        uint16_t producedCargoQuantityDeliveredMonthlyTotal[2];  // 0x19F
        uint16_t producedCargoQuantityDeliveredPreviousMonth[2]; // 0x1A3
        uint8_t producedCargoPercentTransportedPreviousMonth[2]; // 0x1A7 (%)
        uint8_t producedCargoMonthlyHistorySize[2];              // 0x1A9 (<= 20 * 12)
        uint8_t producedCargoMonthlyHistory1[20 * 12];           // 0x1AB (20 years, 12 months)
        uint8_t producedCargoMonthlyHistory2[20 * 12];           // 0x29B
        int32_t history_min_production[2];                       // 0x38B
        uint8_t pad_393[0x453 - 0x393];

        IndustryId id() const;
        const IndustryObject* getObject() const;
        bool empty() const;
        bool canReceiveCargo() const;
        bool canProduceCargo() const;
        void getStatusString(const char* buffer);

        void update();
        void updateDaily();
        void updateMonthly();
        bool isMonthlyProductionUp();
        bool isMonthlyProductionDown();
        bool isMonthlyProductionClosing();
        void sub_45329B(const Map::Pos2& pos);
        void sub_453354();
        void expandGrounds(const Map::Pos2& pos, uint8_t primaryWallType, uint8_t secondaryWallType, uint8_t dl);
        void createMapAnimations();
        void updateProducedCargoStats();
    };
#pragma pack(pop)

    static_assert(sizeof(Industry) == 0x453);
}
