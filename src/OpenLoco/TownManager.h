#pragma once

#include "Town.h"
#include <array>

namespace OpenLoco::townmgr
{
    constexpr size_t max_towns = 80;

    std::array<town, max_towns>& towns();
    town* get(town_id_t id);
    void update();
    void updateLabels();
    void updateMonthly();
}
