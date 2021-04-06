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
    CompanyId_t updatingCompanyId();
    void updatingCompanyId(CompanyId_t id);

    std::array<Company, max_companies>& companies();
    Company* get(CompanyId_t id);
    CompanyId_t getControllingId();
    void setControllingId(CompanyId_t id);
    void setSecondaryPlayerId(CompanyId_t id);
    Company* getPlayerCompany();
    uint8_t getCompanyColour(CompanyId_t id);
    uint8_t getPlayerCompanyColour();
    void update();
    void determineAvailableVehicles();

    struct OwnerStatus
    {
        string_id string;
        uint32_t argument1;
        uint32_t argument2;
    };

    Company* getOpponent();
    string_id getOwnerStatus(CompanyId_t id, FormatArguments& args);
    OwnerStatus getOwnerStatus(CompanyId_t id);
    void updateOwnerStatus();

    void spendMoneyEffect(const Map::map_pos3& loc, const CompanyId_t company, const currency32_t amount);
    void applyPaymentToCompany(const CompanyId_t id, const currency32_t payment, const ExpenditureType type);
}
