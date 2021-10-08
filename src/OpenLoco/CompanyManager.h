#pragma once

#include "Company.h"
#include "Core/LocoFixedVector.hpp"
#include "Limits.h"
#include "Map/Map.hpp"
#include "Types.hpp"
#include <array>
#include <cstddef>

namespace OpenLoco::CompanyManager
{
    void reset();
    CompanyId_t updatingCompanyId();
    void updatingCompanyId(CompanyId_t id);

    FixedVector<Company, Limits::maxCompanies> companies();
    Company* get(CompanyId_t id);
    CompanyId_t getControllingId();
    CompanyId_t getSecondaryPlayerId();
    void setControllingId(CompanyId_t id);
    void setSecondaryPlayerId(CompanyId_t id);
    Company* getPlayerCompany();
    uint8_t getCompanyColour(CompanyId_t id);
    uint8_t getPlayerCompanyColour();
    bool isPlayerCompany(CompanyId_t id);
    void update();
    void updateQuarterly();
    void determineAvailableVehicles();
    currency32_t calculateDeliveredCargoPayment(uint8_t cargoItem, int32_t numUnits, int32_t distance, uint16_t numDays);

    Company* getOpponent();
    string_id getOwnerStatus(CompanyId_t id, FormatArguments& args);
    void updateOwnerStatus();
    void updateColours();
    void setPreferredName();

    void spendMoneyEffect(const Map::Pos3& loc, const CompanyId_t company, const currency32_t amount);
    void applyPaymentToCompany(const CompanyId_t id, const currency32_t payment, const ExpenditureType type);
    uint32_t competingColourMask(CompanyId_t companyId);
    uint32_t competingColourMask();
}
