#pragma once

#include "Currency.h"
#include <cstdint>

namespace OpenLoco::Economy
{
    void updateMonthly();
    void sub_46E2C0(uint16_t year);
    currency32_t getInflationAdjustedCost(int16_t costFactor, uint8_t costIndex, uint8_t divisor);
    void buildDeliveredCargoPaymentsTable();
    uint32_t getCurrencyMultiplicationFactor(uint8_t costIndex);
}
