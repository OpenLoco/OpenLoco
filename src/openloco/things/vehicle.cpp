#include "vehicle.h"
#include <algorithm>
#include "../audio/audio.h"
#include "../config.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../objects/vehicle_object.h"
#include "../objects/objectmgr.h"
#include "../utility/numeric.hpp"
#include "../viewportmgr.h"
#include "thingmgr.h"

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::objectmgr;

loco_global<vehicle *, 0x01136118> vehicle_1136118;
loco_global<vehicle *, 0x01136124> vehicle_1136124;
loco_global<vehicle *, 0x01136128> vehicle_1136128;
loco_global<uint32_t, 0x01136130> vehicle_var_1136130;
loco_global<uint8_t, 0x01136237> vehicle_var_1136237; // var_28 related?
loco_global<uint8_t, 0x01136238> vehicle_var_1136238; // var_28 related?
<<<<<<< a292391919882a169ee318fdd348add509376bb3
loco_global<unk_113D758 *, 0x0113D820> vehicle_var_113D820;
loco_global<uint8_t, 0x0050AF25> vehicle_var_50AF25;
=======

loco_global<uint8_t, 0x0050AF25> vehicle_zoom_max;
>>>>>>> Use OpenRCT2 viewport names

vehicle * vehicle::next_vehicle()
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
        vehicle * veh = vehicle_1136124;
        regs.ebx = (int32_t)veh;
        veh = vehicle_1136128;
        regs.edi = (int32_t)veh;
        call(0x004AC255, regs);
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
    call(0x004AAB0B, regs);
    vehicle_var_1136130 = backup1136130;
    return 0;
}

void openloco::vehicle::sub_4AAC4E()
{
    if (var_38 & (1 << 4))
        return;

    vehicle * veh = vehicle_1136118;
    if ((veh->var_5D == 8) || (veh->var_5D == 9))
        return;

    vehicle_object * vehicleObject = get_vehicle_object(object_type);
    registers regs;
    regs.esi = (int32_t)this;
    regs.bl = vehicleObject->var_24[var_54].var_05;
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

// 0x4CBB01
void openloco::vehicle::invalidate_sprite()
{
    if (sprite_left == 0x8000)
    {
        return;
    }

    int16_t left = sprite_left;
    int16_t top = sprite_top;
    int16_t right = sprite_right;
    int16_t bottom = sprite_bottom;
    for (auto viewport = openloco::ui::viewportmgr::begin();
        viewport != nullptr;
        viewport++)
    {
        if (viewport->zoom > vehicle_zoom_max)
            continue;

        if (sprite_right <= viewport->view_x)
            continue;

        if (sprite_bottom <= viewport->view_y)
            continue;

        if (sprite_left >= viewport->view_x + viewport->view_width)
            continue;

        left = std::max(sprite_left, viewport->view_x);
        right = std::min<int16_t>(sprite_right, viewport->view_x + viewport->view_width);

        if (sprite_top >= viewport->view_y + viewport->view_height)
            continue;

        bottom = std::max(sprite_bottom, viewport->view_y);
        top = std::min<int16_t>(sprite_top, viewport->view_y + viewport->view_height);

        left -= viewport->view_x;
        bottom -= viewport->view_y;
        right -= viewport->view_x;
        top -= viewport->view_y;

        left >>= viewport->zoom;
        bottom >>= viewport->zoom;
        right >>= viewport->zoom;
        top >>= viewport->zoom;

        left += viewport->x;
        bottom += viewport->y;
        right += viewport->x;
        top += viewport->y;

        openloco::gfx::set_dirty_blocks(left, top, right, bottom);
    }

}
