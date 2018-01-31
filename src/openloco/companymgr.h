#pragma once

#include "company.h"
#include <array>
#include <cstddef>

namespace openloco::companymgr
{
    constexpr size_t max_companies = 15;

    company_id_t updating_company_id();
    void updating_company_id(company_id_t id);

    std::array<company, max_companies>& companies();
    company* get(company_id_t id);
    void update();
}
