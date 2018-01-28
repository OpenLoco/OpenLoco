#include "vehicle.h"
#include "../audio/audio.h"
#include "../config.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../objects/objectmgr.h"
#include "../objects/vehicle_object.h"
#include "../openloco.h"
#include "../utility/numeric.hpp"
#include "../viewportmgr.h"
#include "thingmgr.h"
#include <algorithm>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::objectmgr;

loco_global<vehicle*, 0x01136118> vehicle_1136118;
loco_global<vehicle*, 0x01136124> vehicle_1136124;
loco_global<vehicle*, 0x01136128> vehicle_1136128;
loco_global<int32_t, 0x01136130> vehicle_var_1136130;
loco_global<vehicle*, 0x01136120> vehicle_1136120;
loco_global<uint8_t, 0x01136237> vehicle_var_1136237;         // var_28 related?
loco_global<uint8_t, 0x01136238> vehicle_var_1136238;         // var_28 related?
loco_global_array<int8_t, 88, 0x004F865C> vehicle_arr_4F865C; // var_2C related?
loco_global_array<uint16_t, 2047, 0x00500B50> vehicle_arr_500B50;

vehicle* vehicle::next_vehicle()
{
    return thingmgr::get<vehicle>(next_thing_id);
}

vehicle* vehicle::next_car()
{
    return thingmgr::get<vehicle>(next_car_id);
}

vehicle_object* vehicle::object() const
{
    return objectmgr::get<vehicle_object>(object_id);
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
            result = sub_4AA1D0();
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
                    auto soundId = (audio::sound_id)gprng().rand_next(26, 26 + 5);
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

// 0x004AA1D0
int32_t openloco::vehicle::sub_4AA1D0()
{
    registers regs;
    regs.esi = (int32_t)this;

    if (var_42 == 2 || var_42 == 3)
    {
        sub_4AAC4E();
        return 0;
    }

    if (vehicle_var_1136237 | vehicle_var_1136238)
    {
        invalidate_sprite();
        sub_4AC255(vehicle_1136128, vehicle_1136124);
        invalidate_sprite();
    }
    uint32_t backup1136130 = vehicle_var_1136130;
    if (var_5E != 0)
    {
        uint32_t var_1136130 = var_5E;
        if (var_5E > 32)
        {
            var_1136130 = 64 - var_1136130;
        }

        vehicle_var_1136130 += var_1136130 * 320 + 500;
    }
    sub_4AAC4E();
    sub_4AAB0B();
    vehicle_var_1136130 = backup1136130;
    return 0;
}

void openloco::vehicle::sub_4AAC4E()
{
    if (var_38 & (1 << 4))
        return;

    vehicle* veh = vehicle_1136118;
    if ((veh->var_5D == 8) || (veh->var_5D == 9))
        return;

    auto vehicleObject = object();
    registers regs;
    regs.esi = (int32_t)this;
    regs.ebp = (int32_t)vehicleObject;
    regs.ebx = vehicleObject->var_24[var_54].var_05;
    if (vehicleObject->var_24[var_54].var_05 == 0)
    {
        call(0x004AB655, regs);
        return;
    }

    regs.ebx -= 0x80;
    switch (vehicleObject->vis_fx_type)
    {
        case 0:
            call(0x004AB655, regs);
            break;
        case 1:
        case 2:
        case 3:
            call(0x004AACA5, regs);
            break;
        case 4:
            call(0x004AAFFA, regs);
            break;
        case 5:
            call(0x004AB3CA, regs);
            break;
        case 6:
            call(0x004AB4E0, regs);
            break;
        case 7:
            call(0x004AB177, regs);
            break;
        case 8:
            call(0x004AB2A7, regs);
            break;
    }
}

// 0x004AAB0B
void openloco::vehicle::sub_4AAB0B()
{
    int32_t eax = vehicle_var_1136130 >> 3;
    if (var_38 & (1 << 1))
    {
        eax = -eax;
    }

    var_44 += eax & 0xFFFF;
    if (object_sprite_type == 0xFF)
        return;

    auto vehicle_object = object();
    int8_t al = 0;
    if (vehicle_object->sprites[object_sprite_type].flags & (1 << 6))
    {
        vehicle* veh3 = vehicle_1136120;
        al = (veh3->var_56 >> 16) / (vehicle_object->speed / vehicle_object->sprites[object_sprite_type].var_02);
        al = std::min(al, vehicle_object->sprites[object_sprite_type].var_02);
    }
    else if (vehicle_object->sprites[object_sprite_type].var_05 != 1)
    {
        vehicle* veh2 = vehicle_1136124;
        vehicle* veh3 = vehicle_1136120;
        al = var_46;
        int8_t ah = 0;
        if (veh3->var_56 < 0x230000)
        {
            ah = 0;
        }
        else
        {
            ah = vehicle_arr_4F865C[veh2->var_2C >> 2];
            if (((veh2->var_2C >> 3) == 12) || ((veh2->var_2C >> 3) == 13))
            {
                if (veh2->var_2E >= 48)
                {
                    ah = -ah;
                }
            }

            if (ah < 0)
            {
                if (var_38 & (1 << 1))
                {
                    ah = 2;
                    if (al != 0 && al != ah)
                    {
                        ah = 0;
                    }
                }
                else
                {
                    ah = 1;
                    if (al != 0 && al != ah)
                    {
                        ah = 0;
                    }
                }
            }
            else if (ah > 0)
            {
                if (var_38 & (1 << 1))
                {
                    ah = 1;
                    if (al != 0 && al != ah)
                    {
                        ah = 0;
                    }
                }
                else
                {
                    ah = 2;
                    if (al != 0 && al != ah)
                    {
                        ah = 0;
                    }
                }
            }
            else
            {
                ah = 0;
            }
        }
        al = ah;
    }
    else
    {
        al = (var_44 >> 12) & (vehicle_object->sprites[object_sprite_type].var_02 - 1);
    }
    if (var_46 != al)
    {
        var_46 = al;
        invalidate_sprite();
    }
}

// 0x004AC255
void openloco::vehicle::sub_4AC255(vehicle * veh_edi, vehicle * veh_ebx)
{
    loc16 loc = {
        (veh_ebx->x + veh_edi->x) / 2,
        (veh_ebx->y + veh_edi->y) / 2,
        (veh_ebx->z + veh_edi->z) / 2
    };
    move_to(loc);

    if (object_sprite_type == 0xFF)
        return;

    auto distance_x = veh_ebx->x - veh_edi->x;
    auto distance_y = veh_ebx->y - veh_edi->y;
    
    auto offset = sub_4BE368(distance_x * distance_x + distance_y * distance_y);

    auto vehicle_object = object();

    registers regs;
    regs.cx = offset;
    regs.ax = veh_ebx->z - veh_edi->z;
    if (vehicle_object->sprites[object_sprite_type].flags & (1 << 4))
    {
        call(0x004BF4DA, regs);
    }
    else
    {
        call(0x004BF49D, regs);
    }

    var_1F = regs.al;

    regs.ax = distance_x;
    regs.cx = distance_y;
    if (var_1F == 1 || var_1F == 3 || var_1F == 5 ||var_1F == 7)
    {
        call(0x004BF5B3, regs);
    }
    else
    {
        auto sprite = vehicle_object->sprites[object_sprite_type];
        uint8_t i = var_1F == 0 ? sprite.var_0B : sprite.var_0C;
        switch (i)
        {
        case 0:
            call(0x004BF52B, regs);
            break;
        case 1:
            call(0x004BF5B3, regs);
            break;
        case 2:
            call(0x004BF5FB, regs);
            break;
        case 3:
            call(0x004BF657, regs);
            break;
        case 4:
            call(0x004BF6DF, regs);
            break;
        }
    }

    var_1E = regs.al;
}

// 0x004BE368
uint16_t openloco::vehicle::sub_4BE368(uint32_t distance)
{
    uint8_t i = 10;
    for (; distance > 4096; --i, distance >>= 2);

    return vehicle_arr_500B50[distance >> 1] >> i;
}
