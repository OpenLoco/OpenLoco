#include "company.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "openloco.h"
#include <algorithm>
#include <array>

using namespace openloco::interop;

namespace openloco
{
    static loco_global<company_id_t[2], 0x00525E3C> _player_company[2];

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
        return name == string_ids::empty;
    }

    // 0x00430762
    void company::ai_think()
    {
        registers regs;
        regs.esi = (loco_ptr)this;
        call(0x00430762, regs);
    }
}
