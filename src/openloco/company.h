#pragma once

#include <cstdint>
#include <limits>

namespace openloco
{
    using company_id_t = uint8_t;

    namespace company_id
    {
        constexpr company_id_t null = std::numeric_limits<company_id_t>::max();
    }

#pragma pack(push, 1)
    struct company
    {
        uint8_t pad_00[0x8BB0 - 0x00];
        uint8_t var_8BB0[9];
        uint8_t pad_8BB9[0x8FA8 - 0x8BB9];
    };
#pragma pack(pop)

    bool is_player_company(company_id_t id);
}
