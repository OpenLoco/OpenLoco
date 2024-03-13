#pragma once

#include "Engine/Limits.h"
#include "Industry.h"
#include "Objects/IndustryObject.h"
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
    bool hasFlags(const Flags flags);
    void setFlags(const Flags flags);
    void update();
    void updateDaily();
    void updateMonthly();
    int32_t capOfTypeOfIndustry(const uint8_t indObjId);
    void createNewIndustry(const uint8_t indObjId, const bool buildImmediately, const int32_t numAttempts);
    void createAllMapAnimations();
    bool industryNearPosition(const World::Pos2& position, IndustryObjectFlags flags);
    void updateProducedCargoStats();
    IndustryId allocateNewIndustry(const uint8_t type, const World::Pos2& pos, const Core::Prng& prng, const TownId nearbyTown);
}
