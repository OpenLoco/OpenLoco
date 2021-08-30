#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Town.h"
#include <array>
#include "Limits.h"

namespace OpenLoco::TownManager
{
    void reset();
    FixedVector<Town, Limits::maxTowns> towns();
    Town* get(TownId_t id);
    std::optional<std::pair<TownId_t, uint8_t>> getClosestTownAndUnk(const Map::Pos2& loc);
    void update();
    void updateLabels();
    void updateMonthly();
}
