#include "Economy.h"
#include "../CompanyManager.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../Ui/WindowType.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Economy
{
    static const uint32_t kInflationFactors[32] = {
        20,
        20,
        20,
        20,
        23,
        20,
        23,
        23,
        20,
        17,
        17,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
        20,
    };

    static loco_global<currency32_t[32][60], 0x009C68F8> _deliveredCargoPayment;

    static auto& currencyMultiplicationFactors()
    {
        return getGameState().currencyMultiplicationFactor;
    }
    // NB: This is not used for anything due to a mistake in original inflation calculation
    // looks as if it was meant to be extra precesion for the currencyMultiplicationFactor
    // Always 0.
    static auto& unusedCurrencyMultiplicationFactors()
    {
        return getGameState().unusedCurrencyMultiplicationFactor;
    }

    // 0x004375F7
    void buildDeliveredCargoPaymentsTable()
    {
        for (uint8_t cargoItem = 0; cargoItem < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoItem)
        {
            auto* cargoObj = ObjectManager::get<CargoObject>(cargoItem);
            if (cargoObj == nullptr)
            {
                continue;
            }

            for (uint16_t numDays = 2; numDays <= 122; ++numDays)
            {
                _deliveredCargoPayment[cargoItem][(numDays / 2) - 1] = CompanyManager::calculateDeliveredCargoPayment(cargoItem, 100, 10, numDays);
            }
        }
    }

    // 0x0046E239
    // NB: called in sub_46E2C0 below, as well in openloco::date_tick.
    void updateMonthly()
    {
        auto& factors = currencyMultiplicationFactors();
        for (uint8_t i = 0; i < 32; i++)
        {
            factors[i] += (static_cast<uint64_t>(kInflationFactors[i]) * factors[i]) >> 12;
        }

        buildDeliveredCargoPaymentsTable();
        Ui::WindowManager::invalidate(Ui::WindowType::companyList);
        Ui::WindowManager::invalidate(Ui::WindowType::buildVehicle);
        Ui::WindowManager::invalidate(Ui::WindowType::construction);
        Ui::WindowManager::invalidate(Ui::WindowType::terraform);
        Ui::WindowManager::invalidate(Ui::WindowType::industryList);
    }

    // 0x0046E2C0
    void sub_46E2C0(uint16_t year)
    {
        auto& factors = currencyMultiplicationFactors();
        auto& unusedFactors = unusedCurrencyMultiplicationFactors();
        for (uint8_t i = 0; i < 32; i++)
        {
            factors[i] = 1024;
            unusedFactors[i] = 0;
        }

        // OpenLoco allows 1800 as the minimum year, whereas Locomotion uses 1900.
        // Treat years before 1900 as though they were 1900 to not change vanilla scenarios.
        const uint32_t baseYear = std::clamp<uint32_t>(year, 1900, 2030) - 1900;

        for (uint32_t monthCount = baseYear * 12; monthCount > 0; monthCount--)
        {
            updateMonthly();
        }
    }

    currency32_t getInflationAdjustedCost(uint16_t costFactor, uint8_t costIndex, uint8_t divisor)
    {
        return costFactor * currencyMultiplicationFactors()[costIndex] / (1 << divisor);
    }
}
