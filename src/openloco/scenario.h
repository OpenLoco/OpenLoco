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

    enum objective_flags : uint8_t
    {
        flag_0 = (1 << 0),
        be_top_company = (1 << 1),
        be_within_top_three_companies = (1 << 2),
        flag_3 = (1 << 3),
        within_time_limit = (1 << 4),
    };

    enum objective_type : uint8_t
    {
        company_value,
        vehicle_profit,
        performance_index,
        cargo_delivery,
    };

    constexpr uint16_t min_year = 1900;
    constexpr uint16_t max_year = 2100;

    constexpr uint8_t min_objective_year_limit = 2;
    constexpr uint8_t max_objective_year_limit = 100;

    constexpr uint32_t min_objective_company_value = 100000;
    constexpr uint32_t max_objective_company_value = 200000000;

    constexpr uint32_t min_objective_monthly_profit_from_vehicles = 1000;
    constexpr uint32_t max_objective_monthly_profit_from_vehicles = 1000000;

    constexpr uint32_t min_objective_performance_index = 10;
    constexpr uint32_t max_objective_performance_index = 100;

    constexpr uint32_t min_objective_delivered_cargo = 100;
    constexpr uint32_t max_objective_delivered_cargo = 200000000;

    constexpr uint8_t min_competing_companies = 0;
    constexpr uint8_t max_competing_companies = 14;

    constexpr uint8_t min_competitor_start_delay = 0;
    constexpr uint8_t max_competitor_start_delay = 240;

    constexpr uint16_t min_start_loan_units = 50;
    constexpr uint16_t max_start_loan_units = 1250;

    constexpr uint16_t min_loan_size_units = 50;
    constexpr uint16_t max_loan_size_units = 5000;

    constexpr uint16_t min_loan_interest_units = 5;
    constexpr uint16_t max_loan_interest_units = 25;

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
