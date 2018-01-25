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

    bool is_player_company(company_id_t id);
}
