#pragma once

#include "Industry.h"
#include "LocoFixedVector.h"
#include <array>
#include <cstddef>

namespace OpenLoco::IndustryManager
{
    constexpr size_t max_industries = 128;

    void reset();
    LocoFixedVector<Industry> industries();
    Industry* get(IndustryId_t id);
    void update();
    void updateMonthly();
    void createAllMapAnimations();
}
