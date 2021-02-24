#include "Economy.h"
#include "Interop/Interop.hpp"
#include "Ui/WindowManager.h"
#include "Ui/WindowType.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Economy
{
    // Inflation rates?
    static const uint32_t _4FDEC0[32] = {
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

    static loco_global<uint32_t[32], 0x00525E5E> currencyMultiplicationFactor;

    // NB: This is not used for anything due to a mistake in original inflation calculation
    // looks as if it was meant to be extra precesion for the currencyMultiplicationFactor
    // Always 0.
    static loco_global<uint32_t[32], 0x00525EDE> _525EDE;

    // 0x0046E239
    // NB: called in sub_46E2C0 below, as well in openloco::date_tick.
    void sub_46E239()
    {
        for (uint8_t i = 0; i < 32; i++)
        {
            currencyMultiplicationFactor[i] += (static_cast<uint64_t>(_4FDEC0[i]) * currencyMultiplicationFactor[i]) >> 12;
        }

        call(0x004375F7);
        Ui::WindowManager::invalidate(Ui::WindowType::companyList);
        Ui::WindowManager::invalidate(Ui::WindowType::buildVehicle);
        Ui::WindowManager::invalidate(Ui::WindowType::construction);
        Ui::WindowManager::invalidate(Ui::WindowType::terraform);
        Ui::WindowManager::invalidate(Ui::WindowType::industryList);
    }

    // 0x0046E2C0
    void sub_46E2C0(uint16_t year)
    {
        for (uint8_t i = 0; i < 32; i++)
        {
            currencyMultiplicationFactor[i] = 1024;
            _525EDE[i] = 0;
        }

        // OpenLoco allows 1800 as the minimum year, whereas Locomotion uses 1900.
        // Treat years before 1900 as though they were 1900 to not change vanilla scenarios.
        uint32_t yearCount = std::clamp<uint32_t>(year, 1900, 2030) - 1900;

        yearCount *= 12;
        while (yearCount > 0)
        {
            sub_46E239();
            yearCount--;
        }
    }
}
