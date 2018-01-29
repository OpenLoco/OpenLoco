#pragma once

#include "company.h"
#include "localisation/stringmgr.h"
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
        uint8_t pad_02[0x06 - 0x02];
        uint16_t flags;
        uint8_t pad_08[0x30 - 0x08];
        uint32_t population; // 0x30
        uint8_t pad_34[0x3A - 0x34];
        int16_t company_ratings[15];    // 0x3A
        uint16_t companies_with_rating; // 0x58
        town_size size;                 // 0x5A
        uint8_t pad_5B[0x1A6 - 0x5B];
        uint16_t num_stations; // 0x1A6
        uint8_t pad_1A8[0x270 - 0x1A8];

        bool empty() const;
        void adjust_company_rating(company_id_t cid, int amount);
    };
#pragma pack(pop)
}
