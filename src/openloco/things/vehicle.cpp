#include "../audio/audio.h"
#include "../config.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../utility/numeric.hpp"
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

static thing * sub_440BEB(loc16 loc)
{
    auto t = thingmgr::create_thing();
    if (t != nullptr)
    {
        t->var_14 = 44;
        t->var_09 = 32;
        t->var_15 = 34;
        t->var_00 = 1;
        t->move_to(loc);
        t->type = 8;
        t->var_28 = 0;
    }
    return t;
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
                sub_440BEB(loc16(v->x, v->y, v->z + 4));
            }
        }

        if ((v->var_5F & flags_5f::can_breakdown) && !is_title_mode())
        {
            auto newConfig = config::get_new();
            if (!newConfig.breakdowns_disabled)
            {
                v->var_5F &= ~flags_5f::can_breakdown;
                v->var_5F |= flags_5f::flag_2;
                v->var_6A = 5;
                v->sub_4BAA76();

                auto v2 = v->next_car()->next_car();

                // rand_next()
                auto unk18 = LOCO_GLOBAL(0x00525E18, uint32_t);
                auto unk1C = LOCO_GLOBAL(0x00525E1C, uint32_t);
                LOCO_GLOBAL(0x00525E18, uint32_t) = utility::ror(unk1C ^ 0x1234567F, 7);
                LOCO_GLOBAL(0x00525E1C, uint32_t) = utility::ror(unk18, 3);

                auto al = LOCO_GLOBAL(0x00525E1C, uint32_t) & 0xFF;
                auto eax = (audio::sound_id)(26 + (((al * 6) >> 8) & 0xFF));
                audio::play_sound(eax, location<int16_t>(v2->x, v2->y, v2->z + 22));
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

void vehicle::sub_4BAA76()
{
    registers regs;
    regs.esi = (int32_t)this;
    LOCO_CALLPROC_X(0x004BAA76, regs);
}
