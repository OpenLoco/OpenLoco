#include "Town.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Ptr.h"
#include "TownManager.h"
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    constexpr int32_t min_company_rating = -1000;
    constexpr int32_t max_company_rating = 1000;

    bool town::empty() const
    {
        return name == StringIds::null;
    }

    town_id_t town::id() const
    {
        // TODO check if this is stored in town structure
        //      otherwise add it when possible
        auto index = static_cast<size_t>(this - &TownManager::towns()[0]);
        if (index > TownManager::max_towns)
        {
            index = TownId::null;
        }
        return static_cast<town_id_t>(index);
    }

    // 0x0049742F
    void town::update()
    {
        registers regs;
        regs.esi = ToInt(this);
        call(0x0049742F, regs);
    }

    void town::adjustCompanyRating(company_id_t cid, int amount)
    {
        companies_with_rating |= (1 << cid);
        company_ratings[cid] = std::clamp(
            company_ratings[cid] + amount,
            min_company_rating,
            max_company_rating);
    }

    string_id town::getTownSizeString() const
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
