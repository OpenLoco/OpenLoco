#pragma once

#include "company.h"
#include "types.hpp"
#include <array>
#include <cstddef>

namespace openloco::companymgr
{
    constexpr size_t max_companies = 15;

    company_id_t updating_company_id();
    void updating_company_id(company_id_t id);

    std::array<company, max_companies>& companies();
    company* get(company_id_t id);
    company_id_t get_controlling_id();
    uint8_t get_company_colour(company_id_t id);
    uint8_t get_player_company_colour();
    void update();

    struct owner_status
    {
        string_id string;
        uint32_t argument1;
        uint32_t argument2;
    };

    company* getOpponent();
    string_id getOwnerStatus(company_id_t id, FormatArguments& args);
    owner_status getOwnerStatus(company_id_t id);
    void updateOwnerStatus();
}
