#include "company.h"
#include "interop/interop.hpp"
#include <algorithm>
#include <array>

using namespace openloco::interop;

namespace openloco
{
    static loco_global<company_id_t[2], 0x00525E3C> _player_company[2];

    bool is_player_company(company_id_t id)
    {
        auto& player_company = *((std::array<company_id_t, 2>*)_player_company->get());
        auto findResult = std::find(
            player_company.begin(),
            player_company.end(),
            id);
        return findResult != player_company.end();
    }
}
