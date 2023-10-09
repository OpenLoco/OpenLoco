#pragma once

#include "Company.h"
#include "Engine/Limits.h"
#include "Types.hpp"
#include <OpenLoco/Core/LocoFixedVector.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <array>
#include <cstddef>

namespace OpenLoco::CompanyManager
{
    void reset();
    CompanyId getUpdatingCompanyId();
    void setUpdatingCompanyId(CompanyId id);

    uint8_t getMaxCompetingCompanies();
    void setMaxCompetingCompanies(uint8_t competingCompanies);

    uint8_t getCompetitorStartDelay();
    void setCompetitorStartDelay(uint8_t competetorStartDelay);

    uint16_t getMaxLoanSize();
    void setMaxLoanSize(uint16_t loanSize);

    uint16_t getStartingLoanSize();
    void setStartingLoanSize(uint16_t loanSize);

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
    StringId getOwnerStatus(CompanyId id, FormatArguments& args);
    void updateOwnerStatus();
    void updateColours();
    void setPreferredName();

    void spendMoneyEffect(const World::Pos3& loc, const CompanyId company, const currency32_t amount);
    void applyPaymentToCompany(const CompanyId id, const currency32_t payment, const ExpenditureType type);
    uint32_t competingColourMask(CompanyId companyId);
    uint32_t competingColourMask();

    void createPlayerCompany();
    uint8_t getHeadquarterBuildingType();

    // Vector of competitor object index's that are in use that aren't @id's competitor object index.
    std::vector<uint32_t> findAllOtherInUseCompetitors(const CompanyId id);
}
