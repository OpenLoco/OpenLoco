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
    Town* sub_497DC1(const Map::Pos2& loc, uint32_t population, uint32_t unk1, int16_t rating, uint16_t unk3);
    void resetBuildingsInfluence();
    void registerHooks();
}
