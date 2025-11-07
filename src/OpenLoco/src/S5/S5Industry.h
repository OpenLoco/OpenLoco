#pragma once

#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco
{
    struct Industry;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Industry
    {
        uint16_t name;
        coord_t x;                                               // 0x02
        coord_t y;                                               // 0x04
        uint16_t flags;                                          // 0x06
        uint32_t prng0;                                          // 0x08
        uint32_t prng1;                                          // 0x0C
        uint8_t objectId;                                        // 0x10
        uint8_t under_construction;                              // 0x11 (0xFF = Finished)
        uint16_t foundingYear;                                   // 0x12
        uint8_t numTiles;                                        // 0x14
        World::Pos3 tiles[32];                                   // 0x15 bit 15 of z indicates if multiTile (2x2)
        uint16_t town;                                           // 0xD5
        World::Pos2 tileLoop;                                    // 0xD7
        int16_t numFarmTiles;                                    // 0xDB
        int16_t numIdleFarmTiles;                                // 0xDD
        uint8_t productionRate;                                  // 0xDF fraction of dailyTargetProduction out of 256
        uint8_t owner;                                           // 0xE0
        uint32_t stationsInRange[32];                            // 0xE1 each bit represents one station
        uint16_t producedCargoStatsStation[2][4];                // 0x161
        uint8_t producedCargoStatsRating[2][4];                  // 0x171
        uint16_t dailyProductionTarget[2];                       // 0x179
        uint16_t dailyProduction[2];                             // 0x17D
        uint16_t outputBuffer[2];                                // 0x181
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
    };
    static_assert(sizeof(Industry) == 0x453);
#pragma pack(pop)

    S5::Industry exportIndustry(const OpenLoco::Industry& src);
}
