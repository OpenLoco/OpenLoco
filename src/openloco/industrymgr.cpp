#include "industrymgr.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::industrymgr
{
    static loco_global_array<industry, max_industries, 0x005C455C> _industries;

    std::array<industry, max_industries>& industries()
    {
        auto arr = (std::array<industry, max_industries>*)_industries.get();
        return *arr;
    }

    industry* get(industry_id_t id)
    {
        if (id >= _industries.size())
        {
            return nullptr;
        }
        return &_industries[id];
    }
}
