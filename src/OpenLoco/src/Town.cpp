#include "Town.h"
#include "Config.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    bool Town::empty() const
    {
        return name == StringIds::null;
    }

    /**
     * 0x0049742F
     * Update town
     *
     * @param this @<esi>
     */
    void Town::update()
    {
        recalculateSize();

        if (Config::get().townGrowthDisabled)
            return;

        static constexpr std::array<uint8_t, 12> kBuildSpeedToGrowthPerTick = { 0, 1, 3, 5, 7, 9, 12, 16, 22, 0, 0, 0 };
        auto growthPerTick = kBuildSpeedToGrowthPerTick[this->buildSpeed];
        if (growthPerTick == 0 || (growthPerTick == 1 && (gPrng().randNext() & 7)))
        {
            grow(0x07);
        }
        else
        {
            for (int32_t counter = 0; counter < growthPerTick; ++counter)
            {
                grow(0x3F);
            }
        }
    }

    // 0x00497616
    void Town::updateLabel()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x00497616, regs);
    }

    // 0x0049749B
    void Town::updateMonthly()
    {
        // Scroll history
        if (historySize == std::size(history))
        {
            for (size_t i = 0; i < std::size(history) - 1; i++)
                history[i] = history[i + 1];
        }
        else
            historySize++;

        // Compute population growth.
        uint32_t popSteps = std::max<int32_t>(population - historyMinPopulation, 0) / 50;
        uint32_t popGrowth = 0;
        while (popSteps > 255)
        {
            popSteps -= 20;
            popGrowth += 1000;
        }

        // Any population growth to account for?
        if (popGrowth != 0)
        {
            historyMinPopulation += popGrowth;

            uint8_t offset = (popGrowth / 50) & 0xFF;
            for (uint8_t i = 0; i < historySize; i++)
            {
                int16_t newHistory = history[i] - offset;
                history[i] = newHistory >= 0 ? static_cast<uint8_t>(newHistory) : 0;
            }
        }

        // Write new history point.
        auto histIndex = std::clamp<int32_t>(historySize - 1, 0, std::size(history));
        history[histIndex] = popSteps & 0xFF;

        // Find historical maximum population.
        uint8_t maxPopulation = 0;
        for (int i = 0; i < historySize; i++)
            maxPopulation = std::max(maxPopulation, history[i]);

        int32_t popOffset = historyMinPopulation;
        while (maxPopulation <= 235 && popOffset > 0)
        {
            maxPopulation += 20;
            popOffset -= 1000;
        }

        popOffset -= historyMinPopulation;
        if (popOffset != 0)
        {
            popOffset = -popOffset;
            historyMinPopulation -= popOffset;
            popOffset /= 50;

            for (int i = 0; i < historySize; i++)
                history[i] += popOffset;
        }

        // Work towards computing new build speed.
        // will be the smallest of the influence cargo delivered to the town
        // i.e. to get maximum growth max of the influence cargo must be delivered
        // every update. If no influence cargo the grows at max rate
        uint16_t minCargoDelivered = std::numeric_limits<uint16_t>::max();
        uint32_t cargoFlags = cargoInfluenceFlags;
        while (cargoFlags != 0)
        {
            uint32_t cargoId = Utility::bitScanForward(cargoFlags);
            cargoFlags &= ~(1 << cargoId);

            minCargoDelivered = std::min(minCargoDelivered, monthlyCargoDelivered[cargoId]);
        }

        // Compute build speed (1=slow build speed, 4=fast build speed)
        buildSpeed = std::clamp((minCargoDelivered / 100) + 1, 1, 4);

        // Reset all monthlyCargoDelivered intermediaries to zero.
        memset(&monthlyCargoDelivered, 0, sizeof(monthlyCargoDelivered));
    }

    void Town::adjustCompanyRating(CompanyId cid, int amount)
    {
        companiesWithRating |= (1 << enumValue(cid));
        companyRatings[enumValue(cid)] = std::clamp(
            companyRatings[enumValue(cid)] + amount,
            kMinCompanyRating,
            kMaxCompanyRating);
    }

    // 0x004FF714
    constexpr uint32_t _populations[]{ 0, 150, 500, 2500, 8000 };

    /**
     * 0x004975E0
     * Recalculate size
     *
     * @param this @<esi>
     */
    void Town::recalculateSize()
    {
        auto newSize = enumValue(size);
        if (size < TownSize::metropolis
            && populationCapacity >= _populations[enumValue(size) + 1])
        {
            newSize++;
        }
        else if (size != TownSize::hamlet && populationCapacity + 100 < _populations[enumValue(size)])
        {
            newSize--;
        }

        if (static_cast<TownSize>(newSize) != size)
        {
            size = static_cast<TownSize>(newSize);
            Ui::WindowManager::invalidate(Ui::WindowType::townList);
        }
    }

    /**
     * 0x00498116
     * Grow
     *
     * @param this @<esi>
     * @param growFlags @<eax>
     */
    void Town::grow(int32_t growFlags)
    {
        registers regs;
        regs.eax = growFlags;
        regs.esi = X86Pointer(this);
        call(0x00498116, regs);
    }

    string_id Town::getTownSizeString() const
    {
        static string_id townSizeNames[5] = {
            StringIds::town_size_hamlet,
            StringIds::town_size_village,
            StringIds::town_size_town,
            StringIds::town_size_city,
            StringIds::town_size_metropolis
        };

        if (static_cast<uint8_t>(size) < std::size(townSizeNames))
        {
            return townSizeNames[static_cast<uint8_t>(size)];
        }
        return StringIds::town_size_hamlet;
    }
}
