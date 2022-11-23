#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Engine/Limits.h"
#include "Industry.h"
#include <array>
#include <cstddef>

namespace OpenLoco::IndustryManager
{
    namespace Flags
    {
        constexpr uint8_t disallowIndustriesCloseDown = (1 << 0);
        constexpr uint8_t disallowIndustriesStartUp = (1 << 1);
    }

    void reset();
    FixedVector<Industry, Limits::kMaxIndustries> industries();
    Industry* get(IndustryId id);
    uint8_t getFlags();
    void setFlags(const uint8_t flags);
    void update();
    void updateDaily();
    void updateMonthly();
    void createAllMapAnimations();
    bool industryNearPosition(const Map::Pos2& position, uint32_t flags);
    void updateProducedCargoStats();
}
