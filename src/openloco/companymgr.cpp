#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"

using namespace openloco::interop;

namespace openloco::companymgr
{
    static loco_global_array<company, max_companies, 0x00531784> _companies;
    static loco_global<uint8_t, 0x00525FCB> _byte_525FCB;
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;

    static void sub_4306D1();

    company_id_t updating_company_id()
    {
        return _updating_company_id;
    }

    void updating_company_id(company_id_t id)
    {
        _updating_company_id = id;
    }

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

    // 0x00430319
    void update()
    {
        if (!is_editor_mode())
        {
            company_id_t id = scenario_ticks() & 0x0F;
            auto company = get(id);
            if (company != nullptr && !is_player_company(id) && !company->empty())
            {
                updating_company_id(id);
                company->ai_think();
            }

            _byte_525FCB++;
            if (_byte_525FCB >= 192)
            {
                _byte_525FCB = 0;
                sub_4306D1();
            }
        }
    }

    static void sub_4306D1()
    {
        call(0x004306D1);
    }
}
