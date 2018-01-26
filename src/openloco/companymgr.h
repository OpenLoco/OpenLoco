#pragma once

#include "company.h"
#include <array>
#include <cstddef>

namespace openloco::companymgr
{
    constexpr size_t max_companies = 15;

    std::array<company, max_companies>& companies();
    company* get(company_id_t id);
}
