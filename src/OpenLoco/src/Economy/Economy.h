#pragma once

#include "Currency.h"
#include <cstdint>
#include <span>

namespace OpenLoco::Economy
{
    void updateMonthly();
    void sub_46E2C0(uint16_t year);
    currency32_t getInflationAdjustedCost(int16_t costFactor, uint8_t costIndex, uint8_t divisor);
    void buildDeliveredCargoPaymentsTable();
    std::span<currency32_t> getDeliveryCargoPaymentsTable(uint8_t cargoType);
    uint32_t getCurrencyMultiplicationFactor(uint8_t costIndex);
}
