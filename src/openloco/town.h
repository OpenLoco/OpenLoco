#pragma once

#include "company.h"
#include "localisation/stringmgr.h"
#include "map/tile.h"
#include <cstdint>
#include <limits>

namespace openloco
{
    using town_id_t = uint16_t;

    namespace town_id
    {
        constexpr town_id_t null = std::numeric_limits<town_id_t>::max();
    }

    namespace town_flags
    {
        constexpr uint16_t rating_adjusted = 1 << 1;
    }

    enum class town_size : uint8_t
    {
        hamlet,
        village,
        town,
        city,
        metropolis,
    };

#pragma pack(push, 1)
    struct town
    {
        string_id name;
        map::tile_coord_t x;
        map::tile_coord_t y;
        uint16_t flags;
        uint8_t pad_08[0x30 - 0x08];
        uint32_t population; // 0x30
        uint8_t pad_34[0x38 - 0x34];
        uint16_t var_38;
        int16_t company_ratings[15];    // 0x3A
        uint16_t companies_with_rating; // 0x58
        town_size size;                 // 0x5A
        uint8_t history_size;           // 0x5B (<= 20 * 12)
        uint8_t history[20 * 12];       // 0x5C (20 years, 12 months)
        int32_t history_min_population; // 0x14C
        uint8_t pad_150[0x158 - 0x150];
        int16_t monthly_cargo_delivered[32];
        uint32_t cargo_influence_flags;
        uint8_t pad_19C[0x1A4 - 0x19C];
        uint8_t build_speed; // 0x1A4, 1=slow build speed, 4=fast build speed
        uint8_t unk_1A5;
        uint16_t num_stations; // 0x1A6
        uint8_t pad_1A8[0x270 - 0x1A8];

        bool empty() const;
        void update();
        void adjust_company_rating(company_id_t cid, int amount);
    };
    static_assert(sizeof(town) == 0x270);
#pragma pack(pop)
}
