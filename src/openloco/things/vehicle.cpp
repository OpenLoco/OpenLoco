#include "vehicle.h"
#include "../audio/audio.h"
#include "../config.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../utility/numeric.hpp"
#include "thingmgr.h"

using namespace openloco;
using namespace openloco::interop;

vehicle* vehicle::next_vehicle()
{
    return thingmgr::get<vehicle>(next_thing_id);
}

vehicle* vehicle::next_car()
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
            result = call(0x004A8B81, regs);
            break;
        case 1:
            result = call(0x004A9788, regs);
            break;
        case 2:
            result = call(0x004A9B0B, regs);
            break;
        case 3:
            result = call(0x004AA008, regs);
            break;
        case 4:
        case 5:
            result = call(0x004AA1D0, regs);
            break;
        case 6:
            result = call(0x004AA24A, regs);
            break;
    }
    return (result & (1 << 8)) != 0;
}

// 0x00440BEB
static thing* create_black_smoke(loc16 loc)
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
    if (v->type != 6)
    {
        while (true)
        {
            if (v->var_5F & flags_5f::broken_down)
            {
                if ((scenario_ticks() & 3) == 0)
                {
                    auto v2 = v->next_car()->next_car();
                    create_black_smoke(loc16(v2->x, v2->y, v2->z + 4));
                }
            }

            if ((v->var_5F & flags_5f::breakdown_pending) && !is_title_mode())
            {
                auto newConfig = config::get_new();
                if (!newConfig.breakdowns_disabled)
                {
                    v->var_5F &= ~flags_5f::breakdown_pending;
                    v->var_5F |= flags_5f::broken_down;
                    v->var_6A = 5;
                    sub_4BAA76();

                    auto v2 = v->next_car()->next_car();
                    auto soundId = (audio::sound_id)rand_next(26, 26 + 5);
                    audio::play_sound(soundId, loc16(v2->x, v2->y, v2->z + 22));
                }
            }

            v = v->next_car()->next_car()->next_car();
            vehicle* u;
            do
            {
                if (v->type == 6)
                {
                    return;
                }
                u = v->next_car()->next_car();
                if (u->type != 4)
                    v = u->next_car();
            } while (u->type != 4);
        }
    }
}

void vehicle::sub_4BAA76()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004BAA76, regs);
}
