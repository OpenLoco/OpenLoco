#pragma once

#include "Town.h"
#include <array>

namespace OpenLoco::TownManager
{
    constexpr size_t max_towns = 80;

    void reset();
    std::array<Town, max_towns>& towns();
    Town* get(TownId_t id);
    void update();
    void updateLabels();
    void updateMonthly();
}
