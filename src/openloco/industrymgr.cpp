#include "industrymgr.h"
#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"

using namespace openloco;
using namespace openloco::interop;

industrymanager openloco::g_industrymgr;

static loco_global<industry[max_industries], 0x005C455C> _industries;

std::array<industry, max_industries>& industrymanager::industries()
{
    auto arr = (std::array<industry, max_industries>*)_industries.get();
    return *arr;
}

industry* industrymanager::get(industry_id_t id)
{
    if (id >= _industries.size())
    {
        return nullptr;
    }
    return &_industries[id];
}

// 0x00453234
void industrymanager::update(context& ctx)
{
    if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
    {
        ctx.get<companymanager>().updating_company_id(company_id::neutral);
        for (auto& industry : industries())
        {
            if (!industry.empty())
            {
                industry.update(ctx);
            }
        }
    }

    call(0x00453234);
}

// 0x0045383B
void industrymanager::update_monthly()
{
    call(0x0045383B);
}
