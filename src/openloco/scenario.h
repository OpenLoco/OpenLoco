#pragma once

#include <cstdint>

namespace openloco::scenario
{
    enum flags
    {
        landscape_generation_done = (1 << 0),
    };

    constexpr uint16_t min_year = 1900;
    constexpr uint16_t max_year = 2100;

    void generateLandscape();
}
