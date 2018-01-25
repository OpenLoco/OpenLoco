#pragma once

#include "company.h"
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

#pragma pack(push, 1)
    struct town
    {
        int16_t var_00;
        uint8_t pad_02[0x06 - 0x02];
        uint16_t flags;
        uint8_t pad_08[0x3A - 0x08];
        int16_t company_ratings[15];    // 0x3A
        uint16_t companies_with_rating; // 0x58
        uint8_t pad_5A[0x270 - 0x5A];

        bool empty() const;
        void adjust_company_rating(company_id_t cid, int amount);
    };
#pragma pack(pop)
}
