#include "townmgr.h"
#include "companymgr.h"
#include "interop/interop.hpp"
#include "openloco.h"

using namespace openloco;
using namespace openloco::interop;

townmanager openloco::g_townmgr;

static loco_global<town[80], 0x005B825C> _towns;

std::array<town, max_towns>& townmanager::towns()
{
    auto arr = (std::array<town, max_towns>*)_towns.get();
    return *arr;
}

town* townmanager::get(town_id_t id)
{
    if (id >= _towns.size())
    {
        return nullptr;
    }
    return &_towns[id];
}

// 0x00496B6D
void townmanager::update(companymanager& companymgr)
{
    if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
    {
        auto ticks = scenario_ticks();
        if (ticks % 8 == 0)
        {
            town_id_t id = (ticks / 8) % 0x7F;
            auto town = get(id);
            if (town != nullptr && !town->empty())
            {
                companymgr.updating_company_id(company_id::neutral);
                town->update();
            }
        }
    }
}

// 0x0049748C
void townmanager::update_monthly()
{
    call(0x0049748C);
}
