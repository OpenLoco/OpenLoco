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
    std::optional<std::pair<TownId_t, uint8_t>> getClosestTownAndUnk(const Map::Pos2& loc);
    void update();
    void updateLabels();
    void updateMonthly();
}
