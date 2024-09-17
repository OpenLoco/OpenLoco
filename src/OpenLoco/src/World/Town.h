#pragma once

#include "Company.h"
#include "LabelFrame.h"
#include "Map/Tile.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Prng.h>
#include <limits>
#include <optional>

namespace OpenLoco
{
    constexpr int32_t kMinCompanyRating = -1000;
    constexpr int32_t kMaxCompanyRating = 1000;

    enum class TownFlags : uint16_t
    {
        none = 0U,
        sorted = 1U << 0,
        ratingAdjusted = 1U << 1
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TownFlags);

    enum class TownSize : uint8_t
    {
        hamlet,
        village,
        town,
        city,
        metropolis,
    };

    struct RoadExtentResult
    {
        World::Pos3 roadStart;
        uint16_t tad;
        bool isBridge;
    };

#pragma pack(push, 1)
    struct Town
    {
        StringId name;                // 0x00
        coord_t x;                    // 0x02
        coord_t y;                    // 0x04
        TownFlags flags;              // 0x06
        LabelFrame labelFrame;        // 0x08
        Core::Prng prng;              // 0x28
        uint32_t population;          // 0x30
        uint32_t populationCapacity;  // 0x34
        int16_t numBuildings;         // 0x38
        int16_t companyRatings[15];   // 0x3A
        uint16_t companiesWithRating; // 0x58
        TownSize size;                // 0x5A
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

        bool empty() const;
        TownId id() const;
        void update();
        void updateLabel();
        void updateMonthly();
        void adjustCompanyRating(CompanyId cid, int amount);
        void recalculateSize();
        void grow(int32_t growFlags);
        StringId getTownSizeString() const;
        std::optional<RoadExtentResult> findRoadExtent() const;
        void buildInitialRoad();
    };
    static_assert(sizeof(Town) == 0x270);
#pragma pack(pop)
}
