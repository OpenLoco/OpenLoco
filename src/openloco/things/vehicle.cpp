#include "../interop/interop.hpp"
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
