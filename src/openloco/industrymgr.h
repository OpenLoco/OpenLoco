#pragma once

#include "industry.h"
#include <array>
#include <cstddef>

namespace openloco::map
{
    class tilemanager;
}

namespace openloco
{
    class context;

    constexpr size_t max_industries = 128;

    class industrymanager
    {
    public:
        std::array<industry, max_industries>& industries();
        industry* get(industry_id_t id);
        void update(context& ctx);
        void update_monthly();
    };

    extern industrymanager g_industrymgr;
}
