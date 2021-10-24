#include "Town.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "TownManager.h"
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

        static const std::array<uint8_t, 12> buildSpeedToGrowthPerTick = { 0, 1, 3, 5, 7, 9, 12, 16, 22, 0, 0, 0 };
        auto buildSpeed = buildSpeedToGrowthPerTick[this->build_speed];
        if (buildSpeed == 0 || (buildSpeed == 1 && (gPrng().randNext() & 7) == 0))
        {
            grow(0x07);
        }
        else
        {
            for (int32_t counter = 0; counter < buildSpeed; ++counter)
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

    void Town::adjustCompanyRating(CompanyId cid, int amount)
    {
        companies_with_rating |= (1 << enumValue(cid));
        company_ratings[enumValue(cid)] = std::clamp(
            company_ratings[enumValue(cid)] + amount,
            min_company_rating,
            max_company_rating);
    }

    /**
     * 0x004975E0
     * Recalculate size
     *
     * @param this @<esi>
     */
    void Town::recalculateSize()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004975E0, regs);
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
