#include "Town.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "TownManager.h"
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    constexpr int32_t min_company_rating = -1000;
    constexpr int32_t max_company_rating = 1000;

    bool Town::empty() const
    {
        return name == StringIds::null;
    }

    TownId_t Town::id() const
    {
        // TODO check if this is stored in Town structure
        //      otherwise add it when possible
        auto index = static_cast<size_t>(this - &TownManager::towns()[0]);
        if (index > TownManager::max_towns)
        {
            index = TownId::null;
        }
        return static_cast<TownId_t>(index);
    }

    // 0x0049742F
    void Town::update()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x0049742F, regs);
    }

    // 0x00497616
    void Town::updateLabel()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x00497616, regs);
    }

    void Town::adjustCompanyRating(CompanyId_t cid, int amount)
    {
        companies_with_rating |= (1 << cid);
        company_ratings[cid] = std::clamp(
            company_ratings[cid] + amount,
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
