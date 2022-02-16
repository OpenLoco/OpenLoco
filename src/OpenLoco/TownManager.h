#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Limits.h"
#include "Town.h"
#include <array>

namespace OpenLoco::TownManager
{
    void reset();
    FixedVector<Town, Limits::kMaxTowns> towns();
    Town* get(TownId id);
    std::optional<std::pair<TownId, uint8_t>> getClosestTownAndUnk(const Map::Pos2& loc);
    void update();
    void updateLabels();
    void updateMonthly();
}
