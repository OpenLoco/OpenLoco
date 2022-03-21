#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Industry.h"
#include "Limits.h"
#include <array>
#include <cstddef>

namespace OpenLoco::IndustryManager
{
    void reset();
    FixedVector<Industry, Limits::kMaxIndustries> industries();
    Industry* get(IndustryId id);
    void update();
    void updateDaily();
    void updateMonthly();
    void createAllMapAnimations();
    bool industryNearPosition(const Map::Pos2& position, uint32_t flags);
    void updateProducedCargoStats();
}
