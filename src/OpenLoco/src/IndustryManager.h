#pragma once

#include "Engine/Limits.h"
#include "Industry.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/LocoFixedVector.hpp>
#include <array>
#include <cstddef>

namespace OpenLoco::IndustryManager
{
    enum class Flags : uint8_t
    {
        none = 0U,
        disallowIndustriesCloseDown = 1U << 0,
        disallowIndustriesStartUp = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags);

    void reset();
    FixedVector<Industry, Limits::kMaxIndustries> industries();
    Industry* get(IndustryId id);
    Flags getFlags();
    void setFlags(const Flags flags);
    void update();
    void updateDaily();
    void updateMonthly();
    void createAllMapAnimations();
    bool industryNearPosition(const Map::Pos2& position, uint32_t flags);
    void updateProducedCargoStats();
}
