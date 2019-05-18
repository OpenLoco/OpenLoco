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

    constexpr uint8_t min_sea_level = 0;
    constexpr uint8_t max_sea_level = 28;

    constexpr uint8_t min_base_land_height = 0;
    constexpr uint8_t max_base_land_height = 15;

    constexpr uint8_t min_hill_density = 0;
    constexpr uint8_t max_hill_density = 100;

    constexpr uint16_t min_num_forests = 0;
    constexpr uint16_t max_num_forests = 990;

    constexpr uint8_t min_forest_radius = 4;
    constexpr uint8_t max_forest_radius = 40;

    constexpr uint8_t min_forest_density = 1;
    constexpr uint8_t max_forest_density = 7;

    constexpr uint16_t min_num_trees = 0;
    constexpr uint16_t max_num_trees = 20000;

    constexpr uint8_t min_altitude_trees = 0;
    constexpr uint8_t max_altitude_trees = 40;

    void eraseLandscape();
    void generateLandscape();
}
