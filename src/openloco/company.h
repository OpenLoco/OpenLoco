#pragma once

#include <cstdint>
#include <limits>

namespace openloco
{
    using company_id_t = uint8_t;

    namespace company_id
    {
        constexpr company_id_t neutral = 15;
        constexpr company_id_t null = std::numeric_limits<company_id_t>::max();
    }

#pragma pack(push, 1)
    struct company
    {
        uint16_t var_00;
        uint8_t pad_02[0x2579 - 0x02];
        uint8_t var_2579;
        uint16_t var_257A;
        uint16_t var_257C;
        uint8_t pad_257E[0x8BB0 - 0x257E];
        uint8_t var_8BB0[9];
        uint8_t pad_8BB9[0x8FA8 - 0x8BB9];

        company_id_t id() const;
        bool empty() const;
        void ai_think();
    };
#pragma pack(pop)

    bool is_player_company(company_id_t id);
}
