#pragma once

#include "Company.h"
#include "LabelFrame.h"
#include "Map/Tile.h"
#include <limits>

namespace OpenLoco
{
    constexpr int32_t kMinCompanyRating = -1000;
    constexpr int32_t kMaxCompanyRating = 1000;

    namespace TownFlags
    {
        constexpr uint16_t sorted = 1 << 0;
        constexpr uint16_t ratingAdjusted = 1 << 1;
    }

    enum class TownSize : uint8_t
    {
        hamlet,
        village,
        town,
        city,
        metropolis,
    };

#pragma pack(push, 1)
    struct Town
    {
        string_id name;                 // 0x00
        coord_t x;                      // 0x02
        coord_t y;                      // 0x04
        uint16_t flags;                 // 0x06
        LabelFrame labelFrame;          // 0x08
        Utility::prng prng;             // 0x28
        uint32_t population;            // 0x30
        uint32_t populationCapacity;    // 0x34
        int16_t numBuildings;           // 0x38
        int16_t company_ratings[15];    // 0x3A
        uint16_t companies_with_rating; // 0x58
        TownSize size;                  // 0x5A
        uint8_t historySize;            // 0x5B (<= 20 * 12)
        uint8_t history[20 * 12];       // 0x5C (20 years, 12 months)
        int32_t history_min_population; // 0x14C
        uint8_t var_150[8];
        uint16_t monthly_cargo_delivered[32];
        uint32_t cargo_influence_flags;
        uint16_t var_19C[2][2];
        uint8_t build_speed; // 0x1A4, 1=slow build speed, 4=fast build speed
        uint8_t unk_1A5;
        uint16_t num_stations; // 0x1A6
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
        string_id getTownSizeString() const;
    };
    static_assert(sizeof(Town) == 0x270);
#pragma pack(pop)
}
