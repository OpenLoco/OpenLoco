#pragma once

#include "Core/FileSystem.hpp"
#include "Localisation/FormatArguments.hpp"
#include <cstdint>

namespace OpenLoco
{
    enum class MonthId : uint8_t;
}

namespace OpenLoco::Scenario
{
    enum flags
    {
        landscape_generation_done = (1 << 0),
        hills_edge_of_map = (1 << 1),
        preferred_owner_name = (1 << 2),
    };

    enum industry_flags
    {
        disallow_industries_close_down = (1 << 0),
        disallow_industries_start_up = (1 << 1),
    };

    enum objective_flags : uint8_t
    {
        be_top_company = (1 << 0),
        be_within_top_three_companies = (1 << 1),
        within_time_limit = (1 << 2),
        flag_3 = (1 << 3),
        flag_4 = (1 << 4),
    };

    enum objective_type : uint8_t
    {
        company_value,
        vehicle_profit,
        performanceIndex,
        cargo_delivery,
    };

    enum class Season : uint8_t
    {
        autumn = 0,
        winter = 1,
        spring = 2,
        summer = 3,
    };

    // NB: min_year has been changed to 1800 in OpenLoco; Locomotion uses 1900.
    constexpr uint16_t min_year = 1800;
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

    Season nextSeason(Season season);
    void initialiseSnowLine();
    void updateSnowLine(int32_t currentDayOfYear);
    void reset();
    void sub_4748D4();
    void eraseLandscape();
    void generateLandscape();
    void initialiseDate(uint16_t year);

    void initialiseDate(uint16_t year, OpenLoco::MonthId month, uint8_t day);

    /**
     * Loads the given scenario file, but does not initialise any game state.
     */
    bool load(const fs::path& path);

    /**
     * Loads the given scenario file and resets the game state for starting a new scenario.
     */
    bool loadAndStart(const fs::path& path);

    /**
     * Resets the game state (e.g. companies, year, money etc.) for starting a new scenario.
     */
    [[noreturn]] void start();

    void registerHooks();
    void formatChallengeArguments(FormatArguments& args);
    void sub_46115C();
}
