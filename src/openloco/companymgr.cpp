#include "companymgr.h"
#include "interop/interop.hpp"
#include <array>

using namespace openloco::interop;

namespace openloco::companymgr
{
    static loco_global_array<company, max_companies, 0x00531784> _companies;

    std::array<company, max_companies>& companies()
    {
        auto arr = (std::array<company, max_companies>*)_companies.get();
        return *arr;
    }

    company* get(company_id_t id)
    {
        auto index = (size_t)id;
        if (index < _companies.size())
        {
            return &_companies[index];
        }
        return nullptr;
    }
}
