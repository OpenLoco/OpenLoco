#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Town.h"
#include <array>

namespace OpenLoco::TownManager
{
    constexpr size_t max_towns = 80;

    void reset();
    LocoFixedVector<Town> towns();
    Town* get(TownId_t id);
    void update();
    void updateLabels();
    void updateMonthly();
}
