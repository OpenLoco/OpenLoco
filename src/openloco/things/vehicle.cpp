#include "../config.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "thingmgr.h"
#include "vehicle.h"

using namespace openloco;

vehicle * vehicle::next_vehicle()
{
    return thingmgr::get<vehicle>(next_thing_id);
}

vehicle * vehicle::next_car()
{
    return thingmgr::get<vehicle>(next_car_id);
}

void vehicle::update_head()
{
    auto v = this;
    while (v != nullptr)
    {
        if (v->update())
        {
            break;
        }
        v = v->next_car();
    }
}

bool vehicle::update()
{
    int32_t result = 0;
    registers regs;
    regs.esi = (int32_t)this;
    switch (type)
    {
        case 0:
            result = LOCO_CALLPROC_X(0x004A8B81, regs);
            break;
        case 1:
            result = LOCO_CALLPROC_X(0x004A9788, regs);
            break;
        case 2:
            result = LOCO_CALLPROC_X(0x004A9B0B, regs);
            break;
        case 3:
            result = LOCO_CALLPROC_X(0x004AA008, regs);
            break;
        case 4:
        case 5:
            result = LOCO_CALLPROC_X(0x004AA1D0, regs);
            break;
        case 6:
            result = LOCO_CALLPROC_X(0x004AA24A, regs);
            break;
    }
    return (result & (1 << 8)) != 0;
}

// 0x004BA8D4
void vehicle::sub_4BA8D4()
{
    switch (var_5D)
    {
        case 0:
        case 1:
        case 3:
        case 5:
        case 6:
        case 8:
        case 9:
            return;
    }

    auto v = next_car()->next_car()->next_car();
    if (v->type == 6)
    {
        return;
    }

    while (true)
    {
        if (v->var_5F != 4)
        {
            if (!(LOCO_GLOBAL(0x00525F5E, uint32_t) & 3))
            {
                v = v->next_car()->next_car();
                // sub_440BEB(v->x, v->y, v->z + 4);
            }
        }

        if ((var_5F & flags_5f::can_breakdown) && !is_title_mode())
        {
            auto newConfig = config::get_new();
            if (!newConfig.breakdowns_disabled)
            {

            }
        }

        v = v->next_car()->next_car();
        vehicle * u;
        do
        {
            v = v->next_car();
            if (v->type == 6)
            {
                return;
            }
            u = v->next_car()->next_car();
        }
        while (u->type != 4);
    }
}
