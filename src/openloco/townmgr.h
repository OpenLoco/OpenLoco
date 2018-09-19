#pragma once

#include "town.h"
#include <array>

namespace openloco
{
    class companymanager;
}

namespace openloco::townmgr
{
    constexpr size_t max_towns = 80;

    std::array<town, max_towns>& towns();
    town* get(town_id_t id);
    void update(companymanager& companymgr);
    void update_monthly();
}
