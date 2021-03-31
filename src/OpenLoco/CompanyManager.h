#pragma once

#include "Company.h"
#include "Map/Map.hpp"
#include "Types.hpp"
#include <array>
#include <cstddef>

namespace OpenLoco::CompanyManager
{
    constexpr size_t max_companies = 15;

    void reset();
    company_id_t updatingCompanyId();
    void updatingCompanyId(company_id_t id);

    std::array<company, max_companies>& companies();
    company* get(company_id_t id);
    company_id_t getControllingId();
    company* getPlayerCompany();
    uint8_t getCompanyColour(company_id_t id);
    uint8_t getPlayerCompanyColour();
    void update();
    void determineAvailableVehicles();

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

    void spendMoneyEffect(const Map::map_pos3& loc, const company_id_t company, const currency32_t amount);
}
