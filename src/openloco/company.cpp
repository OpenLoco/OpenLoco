#include "company.h"
#include "interop/interop.hpp"
#include <algorithm>
#include <array>

using namespace openloco::interop;

namespace openloco
{
    static loco_global_array<company_id_t, 2, 0x00525E3C> _player_company[2];

    bool is_player_company(company_id_t id)
    {
        auto& player_company = *((std::array<company_id_t, 2>*)_player_company->get());
        auto findResult = std::find(
            player_company.begin(),
            player_company.end(),
            id);
        return findResult != player_company.end();
    }

    company_id_t company::id() const
    {
        auto first = (company*)0x00531784;
        return (company_id_t)(this - first);
    }

    bool company::empty() const
    {
        return var_00 == 0;
    }

    // 0x00430762
    void company::ai_think()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x00430762, regs);
    }
}
