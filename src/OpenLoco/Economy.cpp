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

    // NB: _525EDE[2] is dword_525E66, referred to by four subs for (price) computation.
    static loco_global<uint32_t[32], 0x00525EDE> _525EDE;

    // 0x0046E239
    // NB: called in sub_46E2C0 below, as well in openloco::date_tick.
    void sub_46E239()
    {
        for (uint8_t i = 0; i < 32; i++)
        {
            uint32_t ebx = currencyMultiplicationFactor[i];

            // Inflating prices?
            uint64_t edx_eax = _525EDE[i] * _4FDEC0[i];
            edx_eax >>= 12;

            // Storing two halves of the money struct?
            _525EDE[i] += static_cast<uint32_t>(edx_eax);
            currencyMultiplicationFactor[i] += static_cast<uint32_t>(edx_eax >> 32);

            // Inflating prices more?
            edx_eax = ebx * _4FDEC0[i];
            edx_eax >>= 12;
            currencyMultiplicationFactor[i] += static_cast<uint32_t>(edx_eax);
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
