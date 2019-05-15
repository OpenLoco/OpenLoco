#pragma once

#include <cstdint>

namespace openloco::scenario
{
    enum flags
    {
        landscape_generation_done = (1 << 0),
        hills_edge_of_map = (1 << 1),
    };

    enum industry_flags
    {
        allow_industries_close_down = (1 << 0),
        allow_industries_start_up = (1 << 1),
    };

    constexpr uint16_t min_year = 1900;
    constexpr uint16_t max_year = 2100;

    void eraseLandscape();
    void generateLandscape();
}
