#include "town.h"
#include <algorithm>

namespace openloco
{
    constexpr int32_t min_company_rating = -1000;
    constexpr int32_t max_company_rating = 1000;

    bool town::empty() const
    {
        return var_00 != -1;
    }

    void town::adjust_company_rating(company_id_t cid, int amount)
    {
        companies_with_rating |= (1 << cid);
        company_ratings[cid] = std::clamp(
            company_ratings[cid] + amount,
            min_company_rating,
            max_company_rating);
    }
}
