#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Interop/Interop.hpp"
#include "OpenLoco.h"

using namespace OpenLoco::interop;

namespace OpenLoco::industrymgr
{
    static loco_global<industry[max_industries], 0x005C455C> _industries;

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

    // 0x00453234
    void update()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !isEditorMode())
        {
            companymgr::updatingCompanyId(company_id::neutral);
            for (auto& industry : industries())
            {
                if (!industry.empty())
                {
                    industry.update();
                }
            }
        }
    }

    // 0x0045383B
    void updateMonthly()
    {
        call(0x0045383B);
    }
}
