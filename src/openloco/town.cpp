#include "town.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include <algorithm>

using namespace openloco::interop;

namespace openloco
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
        regs.esi = (loco_ptr)this;
        call(0x0049742F, regs);
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
