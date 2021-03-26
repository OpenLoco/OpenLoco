#pragma once

#include "Company.h"
#include "Localisation/StringManager.h"
#include "Map/Tile.h"
#include "ZoomLevel.hpp"
#include <cstdint>
#include <limits>

namespace OpenLoco
{
#pragma pack(push, 1)
    struct LabelPosition
    {
        int16_t left[ZoomLevel::max]{};
        int16_t right[ZoomLevel::max]{};
        int16_t top[ZoomLevel::max]{};
        int16_t bottom[ZoomLevel::max]{};

        [[nodiscard]] bool contains(OpenLoco::Ui::Rect& rec, uint8_t zoom) const
        {
            if (rec.top() > bottom[zoom])
            {
                return false;
            }
            if (rec.bottom() < top[zoom])
            {
                return false;
            }
            if (rec.left() > right[zoom])
            {
                return false;
            }
            if (rec.right() < left[zoom])
            {
                return false;
            }
            return true;
        }
    };
#pragma pack(pop)

    namespace TownId
    {
        constexpr town_id_t null = std::numeric_limits<town_id_t>::max();
    }

    namespace TownFlags
    {
        constexpr uint16_t sorted = 1 << 0;
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
        string_id name;              // 0x00
        coord_t x;                   // 0x02
        coord_t y;                   // 0x04
        uint16_t flags;              // 0x06
        LabelPosition labelPosition; // 0x08
        uint8_t pad_28[0x30 - 0x28];
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
        uint16_t monthly_cargo_delivered[32];
        uint32_t cargo_influence_flags;
        uint8_t pad_19C[0x1A4 - 0x19C];
        uint8_t build_speed; // 0x1A4, 1=slow build speed, 4=fast build speed
        uint8_t unk_1A5;
        uint16_t num_stations; // 0x1A6
        uint8_t pad_1A8[0x270 - 0x1A8];

        bool empty() const;
        town_id_t id() const;
        void update();
        void updateLabel();
        void adjustCompanyRating(company_id_t cid, int amount);
        string_id getTownSizeString() const;
    };
    static_assert(sizeof(town) == 0x270);
#pragma pack(pop)
}
