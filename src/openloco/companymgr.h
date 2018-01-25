#pragma once

#include "company.h"
#include <cstddef>

namespace openloco::companymgr
{
    constexpr size_t max_companies = 15;

    company* get(company_id_t id);
}
