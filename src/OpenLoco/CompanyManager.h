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
    CompanyId getUpdatingCompanyId();
    void setUpdatingCompanyId(CompanyId id);

    FixedVector<Company, Limits::kMaxCompanies> companies();
    Company* get(CompanyId id);
    CompanyId getControllingId();
    CompanyId getSecondaryPlayerId();
    void setControllingId(CompanyId id);
    void setSecondaryPlayerId(CompanyId id);
    Company* getPlayerCompany();
    Colour getCompanyColour(CompanyId id);
    Colour getPlayerCompanyColour();
    bool isPlayerCompany(CompanyId id);
    void update();
    void updateDaily();
    void updateMonthly1();
    void updateMonthlyHeadquarters();
    void updateQuarterly();
    void updateYearly();
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

    void createPlayerCompany();
    uint8_t getHeadquarterBuildingType();
}
