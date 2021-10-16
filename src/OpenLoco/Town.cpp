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

    // 0x0049742F
    void Town::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004975E0, regs); // 0049742F

        static const std::array<uint8_t, 12> buildSpeedTable = { 0, 1, 3, 5, 7, 9, 12, 16, 22, 0, 0, 0 }; // byte_4FF728
        int32_t buildSpeed = static_cast<int32_t>(buildSpeedTable[this->build_speed]);                    // 00497434-0049743B
        if (buildSpeed == 0 || (buildSpeed == 1 && (gPrng().randNext() & 7) == 0))                        // 00497442-0049746E
        {
            regs.eax = 7;           // 00497481
            call(0x00498116, regs); // 00497486
        }
        else
        {
            for (int32_t counter = 0; counter < buildSpeed; ++counter) // 0049747C-0049747D
            {
                regs.eax = 0x3F;        // 00497471
                call(0x00498116, regs); // 00497476
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
