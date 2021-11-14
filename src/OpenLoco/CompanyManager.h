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
    CompanyId updatingCompanyId();
    void updatingCompanyId(CompanyId id);

    FixedVector<Company, Limits::maxCompanies> companies();
    Company* get(CompanyId id);
    CompanyId getControllingId();
    CompanyId getSecondaryPlayerId();
    void setControllingId(CompanyId id);
    void setSecondaryPlayerId(CompanyId id);
    Company* getPlayerCompany();
    uint8_t getCompanyColour(CompanyId id);
    uint8_t getPlayerCompanyColour();
    bool isPlayerCompany(CompanyId id);
    void update();
    void updateQuarterly();
    void determineAvailableVehicles();
    currency32_t calculateDeliveredCargoPayment(uint8_t cargoItem, int32_t numUnits, int32_t distance, uint16_t numDays);

    Company* getOpponent();
    string_id getOwnerStatus(CompanyId id, FormatArguments& args);
    void updateOwnerStatus();
    void updateColours();
    void setPreferredName();

    void spendMoneyEffect(const Map::Pos3& loc, const CompanyId company, const currency32_t amount);
    void applyPaymentToCompany(const CompanyId id, const currency32_t payment, const ExpenditureType type);
    uint32_t competingColourMask(CompanyId companyId);
    uint32_t competingColourMask();

    void sub_42F863();
}
