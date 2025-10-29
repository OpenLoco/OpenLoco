#pragma once

#include "S5/S5LabelFrame.h"
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco
{
    struct Town;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Town
    {
        uint16_t name;                // 0x00
        coord_t x;                    // 0x02
        coord_t y;                    // 0x04
        uint16_t flags;               // 0x06
        LabelFrame labelFrame;        // 0x08
        uint32_t prng0;               // 0x28
        uint32_t prng1;               // 0x2C
        uint32_t population;          // 0x30
        uint32_t populationCapacity;  // 0x34
        int16_t numBuildings;         // 0x38
        int16_t companyRatings[15];   // 0x3A
        uint16_t companiesWithRating; // 0x58
        uint8_t size;                 // 0x5A
        uint8_t historySize;          // 0x5B (<= 20 * 12)
        uint8_t history[20 * 12];     // 0x5C (20 years, 12 months)
        int32_t historyMinPopulation; // 0x14C
        uint8_t var_150[8];
        uint16_t monthlyCargoDelivered[32]; // 0x158
        uint32_t cargoInfluenceFlags;       // 0x198
        uint16_t var_19C[2][2];
        uint8_t buildSpeed;       // 0x1A4, 1=slow build speed, 4=fast build speed
        uint8_t numberOfAirports; // 0x1A5
        uint16_t numStations;     // 0x1A6
        uint32_t var_1A8;
        uint8_t pad_1AC[0x270 - 0x1AC];
    };
#pragma pack(pop)

    S5::Town exportTown(OpenLoco::Town& src);
}
