#include "Town.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    constexpr int32_t min_company_rating = -1000;
    constexpr int32_t max_company_rating = 1000;

    bool town::empty() const
    {
        return name == string_ids::null;
    }

    // 0x0049742F
    void town::update()
    {
        registers regs;
        regs.esi = (int32_t)this;
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
            string_ids::town_size_hamlet,
            string_ids::town_size_village,
            string_ids::town_size_town,
            string_ids::town_size_city,
            string_ids::town_size_metropolis
        };

        if (static_cast<uint8_t>(size) < std::size(townSizeNames))
        {
            return townSizeNames[static_cast<uint8_t>(size)];
        }
        return string_ids::town_size_hamlet;
    }
}
