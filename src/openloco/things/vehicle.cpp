#include "vehicle.h"
#include "../audio/audio.h"
#include "../config.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../map/tilemgr.h"
#include "../objects/objectmgr.h"
#include "../objects/vehicle_object.h"
#include "../openloco.h"
#include "../utility/numeric.hpp"
#include "../viewportmgr.h"
#include "misc.h"
#include "thingmgr.h"
#include <algorithm>
#include <cassert>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::objectmgr;

loco_global<vehicle*, 0x01136118> vehicle_1136118; // type 0
loco_global<vehicle*, 0x0113611C> vehicle_113611C; // type 1
loco_global<vehicle_2*, 0x01136120> vehicle_1136120; // type 2
loco_global<vehicle*, 0x01136124> vehicle_front_bogie;
loco_global<vehicle*, 0x01136128> vehicle_back_bogie;
loco_global<int32_t, 0x0113612C> vehicle_var_113612C;
loco_global<int32_t, 0x01136130> vehicle_var_1136130;
loco_global<uint8_t, 0x01136237> vehicle_var_1136237;   // var_28 related?
loco_global<uint8_t, 0x01136238> vehicle_var_1136238;   // var_28 related?
loco_global<uint8_t, 0x0113646C> vehicle_var_113646C;  // var_5D retated? used only in 4B980A
loco_global<int8_t[88], 0x004F865C> vehicle_arr_4F865C; // var_2C related?
loco_global<uint16_t[2047], 0x00500B50> vehicle_arr_500B50;
loco_global<int16_t[128], 0x00503B6A> factorXY503B6A;
loco_global<uint8_t[44], 0x004F8A7C> vehicle_arr_4F8A7C; // bools
loco_global<uint8_t, 0x00525FAE> vehicle_var_525FAE;     // boolean

// 0x00503E5C
static constexpr uint8_t vehicleBodyIndexToPitch[] = {
    0,
    1,
    2,
    3,
    4,
    0, // Not a straight number count
    5,
    6,
    7,
    8
};

// 0x00503B50
constexpr int16_t factor503B50[] = {
    0,
    -26,
    -52,
    -75,
    -98,
    26,
    52,
    75,
    98,
    -44,
    44,
    -84,
    84
};

vehicle* vehicle::next_vehicle()
{
    return thingmgr::get<vehicle>(next_thing_id);
}

vehicle* vehicle::next_car()
{
    return thingmgr::get<vehicle>(next_car_id);
}

thing* vehicle_2::next_car()
{
    return thingmgr::get<thing>(next_car_id);
}

thing* vehicle_26::next_car()
{
    return thingmgr::get<thing>(next_car_id);
}

vehicle_object* vehicle::object() const
{
    return objectmgr::get<vehicle_object>(object_id);
}

vehicle_object* vehicle_2::object() const
{
    return objectmgr::get<vehicle_object>(object_id);
}

vehicle_object* vehicle_26::object() const
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
        case thing_type::exhaust:
            result = sub_4A8B81();
            break;
        case thing_type::vehicle_1:
            result = call(0x004A9788, regs);
            break;
        case thing_type::vehicle_2:
            result = call(0x004A9B0B, regs);
            break;
        case thing_type::vehicle_bogie:
            result = call(0x004AA008, regs);
            break;
        case thing_type::vehicle_body_end:
        case thing_type::vehicle_body_cont:
            result = sub_4AA1D0();
            break;
        case thing_type::vehicle_6:
            result = call(0x004AA24A, regs);
            break;
        default:
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
    if (v->type != thing_type::vehicle_6)
    {
        while (true)
        {
            if (v->var_5F & flags_5f::broken_down)
            {
                if ((scenario_ticks() & 3) == 0)
                {
                    auto v2 = v->next_car()->next_car();
                    smoke::create(loc16(v2->x, v2->y, v2->z + 4));
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
                if (v->type == thing_type::vehicle_6)
                {
                    return;
                }
                u = v->next_car()->next_car();
                if (u->type != thing_type::vehicle_body_end)
                    v = u->next_car();
            } while (u->type != thing_type::vehicle_body_end);
        }
    }
}

void vehicle::sub_4BAA76()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004BAA76, regs);
}

int32_t openloco::vehicle::sub_4A8B81()
{
    vehicle_1136118 = this;
    auto veh = next_car();
    vehicle_113611C = veh;
    veh = veh->next_car();
    vehicle_1136120 = ((thing*)veh)->as_vehicle_2();

    vehicle_var_113646C = var_5D;
    sub_4A8882();


    vehicle_front_bogie = (vehicle*)0xFFFFFFFF;
    vehicle_back_bogie = (vehicle*)0xFFFFFFFF;

    vehicle_2* vehType2 = vehicle_1136120;
    vehicle_var_113612C = vehType2->var_56 >> 7;
    vehicle_var_1136130 = vehType2->var_56 >> 7;

    if (var_5C != 0)
    {
        var_5C--;
    }

    if (tile_x == -1)
    {
        //4A8F3F
        vehicle_6* vehType6 = vehType2->next_car()->as_vehicle_6();
        if (vehType6 == nullptr)
        {
            return 0x10;
        }

        vehType6->var_4F++;
        if (vehType6->var_4F < 960)
        {
            return 0x10;
        }
        sub_4AF06E();
        return 0x10;
    }
    sub_4BA8D4();
    //4A8BF8
    return int32_t();
}

// 0x004AA1D0
int32_t openloco::vehicle::sub_4AA1D0()
{
    registers regs;
    regs.esi = (int32_t)this;

    if (var_42 == 2 || var_42 == 3)
    {
        animation_update();
        return 0;
    }

    if (vehicle_var_1136237 | vehicle_var_1136238)
    {
        invalidate_sprite();
        sub_4AC255(vehicle_back_bogie, vehicle_front_bogie);
        invalidate_sprite();
    }
    uint32_t backup1136130 = vehicle_var_1136130;
    if (var_5E != 0)
    {
        int32_t var_1136130 = var_5E;
        if (var_5E > 32)
        {
            var_1136130 = 64 - var_1136130;
        }

        vehicle_var_1136130 += var_1136130 * 320 + 500;
    }
    animation_update();
    sub_4AAB0B();
    vehicle_var_1136130 = backup1136130;
    return 0;
}

void openloco::vehicle::animation_update()
{
    if (var_38 & (1 << 4))
        return;

    vehicle* veh = vehicle_1136118;
    if ((veh->var_5D == 8) || (veh->var_5D == 9))
        return;

    auto vehicleObject = object();

    if (vehicleObject->var_24[var_54].var_05 == 0)
    {
        return;
    }

    switch (vehicleObject->animation[0].type)
    {
        case simple_animation_type::none:
            break;
        case simple_animation_type::steam_puff1:
        case simple_animation_type::steam_puff2:
        case simple_animation_type::steam_puff3:
            steam_puffs_animation_update(0, vehicleObject->var_24[var_54].var_05 - 0x80);
            break;
        case simple_animation_type::diesel_exhaust1:
            diesel_exhaust1_animation_update(0, vehicleObject->var_24[var_54].var_05 - 0x80);
            break;
        case simple_animation_type::electric_spark1:
            electric_spark1_animation_update(0, vehicleObject->var_24[var_54].var_05 - 0x80);
            break;
        case simple_animation_type::electric_spark2:
            electric_spark2_animation_update(0, vehicleObject->var_24[var_54].var_05 - 0x80);
            break;
        case simple_animation_type::diesel_exhaust2:
            diesel_exhaust2_animation_update(0, vehicleObject->var_24[var_54].var_05 - 0x80);
            break;
        case simple_animation_type::ship_wake:
            ship_wake_animation_update(0, vehicleObject->var_24[var_54].var_05 - 0x80);
            break;
        default:
            assert(false);
            break;
    }
    secondary_animation_update();
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
        vehicle_2* vehType2 = vehicle_1136120;
        al = (vehType2->var_56 >> 16) / (vehicle_object->speed / vehicle_object->sprites[object_sprite_type].var_02);
        al = std::min(al, vehicle_object->sprites[object_sprite_type].var_02);
    }
    else if (vehicle_object->sprites[object_sprite_type].var_05 != 1)
    {
        vehicle* frontBogie = vehicle_front_bogie;
        vehicle_2* vehType2 = vehicle_1136120;
        al = var_46;
        int8_t ah = 0;
        if (vehType2->var_56 < 0x230000)
        {
            ah = 0;
        }
        else
        {
            ah = vehicle_arr_4F865C[frontBogie->var_2C >> 2];
            if (((frontBogie->var_2C >> 3) == 12) || ((frontBogie->var_2C >> 3) == 13))
            {
                if (frontBogie->var_2E >= 48)
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

void openloco::vehicle::sub_4A8882()
{
    vehicle_2 * vehType2 = vehicle_1136120;
    sub_4A88A6((vehicle_26*)vehType2);
    vehicle * vehType6 = (vehicle *)vehType2;
    while (vehType6->type != thing_type::vehicle_6)
    {
        vehType6 = vehType6->next_car();
    }
    sub_4A88A6((vehicle_26*)vehType6);
}

// Not guaranteed to be type 2 could be type 6
void openloco::vehicle::sub_4A88A6(vehicle_26 * vehType2or6)
{
    if (tile_x == 0xFFFF || 
        var_5D == 8 || 
        var_5D == 9 || 
        (var_38 & (1<<4))||
        vehType2or6->object_id == 0xFFFF)
    {
        sub_4A8B7C(vehType2or6);
        return;
    }

    auto vehicleObject = vehType2or6->object();
    switch (vehicleObject->startsnd_type)
    {
    case 0:
        sub_4A8B7C(vehType2or6);
        break;
    case 1:
        sub_4A88F7(vehType2or6, &vehicleObject->sound.type_1);
        break;
    case 2:
        sub_4A8937(vehType2or6, &vehicleObject->sound.type_2);
        break;
    case 3:
        sub_4A8A39(vehType2or6, &vehicleObject->sound.type_3);
        break;
    default:
        assert(false);
    }
}

void openloco::vehicle::sub_4A8B7C(vehicle_26 * vehType2or6)
{
    vehType2or6->var_44 = 0xFF;
}

void openloco::vehicle::sub_4A88F7(vehicle_26 * vehType2or6, vehicle_object_sound_1 * snd)
{
    vehicle_2 * vehType2_2 = vehicle_1136120;
    if (vehType2_2->var_56 < snd->var_01) {
        sub_4A8B7C(vehType2or6);
        return;
    }

    uint32_t unk = vehType2_2->var_56 - snd->var_01;
    vehType2or6->var_46 = (unk >> snd->var_05) + snd->var_06;

    unk >>= snd->var_08;
    unk += snd->var_09;

    if (unk > snd->var_0A)
    {
        unk = snd->var_0A;
    }

    vehType2or6->var_45 = unk;
    vehType2or6->var_44 = snd->var_00;
}

void openloco::vehicle::sub_4A8937(vehicle_26 * vehType2or6, vehicle_object_sound_2 * snd)
{
    if ((vehicle_2*)vehType2or6 == vehicle_1136120)
    {
        if (var_5E != 5 && var_5E != 4)
        {
            // Can be a type 6 or bogie
            if (((vehicle *)vehType2or6->next_car())->var_5F & (1 << 2))
            {
                sub_4A8B7C(vehType2or6);
                return;
            }
        }
    }

    vehicle_2 * vehType2_2 = vehicle_1136120;
    uint16_t _var_46 = 0;
    uint8_t _var_45 = 0;
    if (vehType2_2->var_5A == 2)
    {
        if (vehType2_2->var_56 < 786432)
        {
            _var_46 = snd->var_01;
            _var_45 = snd->var_03;
        }
        else
        {
            _var_46 = snd->var_04;
            _var_45 = snd->var_06;
        }
    }
    else if (vehType2_2->var_5A == 1)
    {
        if (vehType2or6->type == thing_type::vehicle_2 ||
            ((vehicle *)vehType2or6->next_car())->var_5E == 0)
        {
            _var_46 = snd->var_07 + (vehType2_2->var_56 >> snd->var_10);
            _var_45 = snd->var_09;
        }
        else
        {
            _var_46 = snd->var_01;
            _var_45 = snd->var_03;
        }
    }
    else
    {
        _var_46 = snd->var_01;
        _var_45 = snd->var_03;
    }

    if (vehType2or6->var_44 == 0xFF)
    {
        // Half
        vehType2or6->var_45 = snd->var_03 >> 1;
        // Quarter
        vehType2or6->var_46 = snd->var_01 >> 2;
        vehType2or6->var_44 = snd->var_00;
        return;
    }

    if (vehType2or6->var_46 != _var_46)
    {
        if (vehType2or6->var_46 > _var_46)
        {
            vehType2or6->var_46 = std::max(_var_46, (uint16_t)(vehType2or6->var_46 - snd->var_0C));
        }
        else
        {
            vehType2or6->var_46 = std::min(_var_46, (uint16_t)(vehType2or6->var_46 + snd->var_0A));
        }
    }
    
    if (vehType2or6->var_45 != _var_45)
    {
        if (vehType2or6->var_45 > _var_45)
        {
            vehType2or6->var_45 = std::max(_var_45, (uint8_t)(vehType2or6->var_45 - snd->var_0F));
        }
        else
        {
            vehType2or6->var_45 = std::min(_var_45, (uint8_t)(vehType2or6->var_45 + snd->var_0E));
        }
    }

    vehType2or6->var_44 = snd->var_00;
}

void openloco::vehicle::sub_4A8A39(vehicle_26 * vehType2or6, vehicle_object_sound_3 * snd)
{
    if ((vehicle_2*)vehType2or6 == vehicle_1136120)
    {
        if (var_5E != 5 && var_5E != 4)
        {
            // Can be a type 6 or bogie
            if (((vehicle *)vehType2or6->next_car())->var_5F & (1 << 2))
            {
                sub_4A8B7C(vehType2or6);
                return;
            }
        }
    }

    vehicle_2 * vehType2_2 = vehicle_1136120;
    uint16_t _var_46 = 0;
    uint8_t _var_45 = 0;
    bool var5aEqual1Code = false;

    if (vehType2_2->var_5A == 2 || vehType2_2->var_5A == 3)
    {
        if (vehType2_2->var_56 < 786432)
        {
            _var_46 = snd->var_01;
            _var_45 = snd->var_12;
        }
        else
        {
            _var_45 = snd->var_12;
            var5aEqual1Code = true;
        }
    }
    else if (vehType2_2->var_5A == 1)
    {
        _var_45 = snd->var_13;
        var5aEqual1Code = true;

    }
    else
    {
        _var_46 = snd->var_01;
        _var_45 = snd->var_03;
    }

    if (var5aEqual1Code == true)
    {
        if (vehType2or6->type == thing_type::vehicle_2 ||
            ((vehicle *)vehType2or6->next_car())->var_5E == 0)
        {
            auto _var_56 = std::min(vehType2_2->var_56, (uint32_t)458752) >> 16;

            auto dx = snd->var_04;

            if (_var_56 >= snd->var_06)
            {
                dx -= snd->var_08;
                if (_var_56 >= snd->var_0A)
                {
                    dx -= snd->var_0C;
                    if (_var_56 >= snd->var_0E)
                    {
                        dx -= snd->var_10;
                    }
                }
            }
            _var_56 <<= 16;
            _var_46 = (uint16_t)((_var_56 >> snd->var_1A) + dx);
        }
        else
        {
            _var_46 = snd->var_01;
            _var_45 = snd->var_03;
        }
    }

    if (vehType2or6->var_44 == 0xFF)
    {
        // Half
        vehType2or6->var_45 = snd->var_03 >> 1;
        // Quarter
        vehType2or6->var_46 = snd->var_01 >> 2;
        vehType2or6->var_44 = snd->var_00;
        return;
    }

    if (vehType2or6->var_46 != _var_46)
    {
        if (vehType2or6->var_46 > _var_46)
        {
            _var_45 = snd->var_12;
            vehType2or6->var_46 = std::max(_var_46, (uint16_t)(vehType2or6->var_46 - snd->var_16));
        }
        else
        {
            vehType2or6->var_46 = std::min(_var_46, (uint16_t)(vehType2or6->var_46 + snd->var_14));
        }
    }

    if (vehType2or6->var_45 != _var_45)
    {
        if (vehType2or6->var_45 > _var_45)
        {
            vehType2or6->var_45 = std::max(_var_45, (uint8_t)(vehType2or6->var_45 - snd->var_19));
        }
        else
        {
            vehType2or6->var_45 = std::min(_var_45, (uint8_t)(vehType2or6->var_45 + snd->var_18));
        }
    }

    vehType2or6->var_44 = snd->var_00;
}

// 0x004AC255
void openloco::vehicle::sub_4AC255(vehicle* back_bogie, vehicle* front_bogie)
{
    loc16 loc = {
        static_cast<int16_t>((front_bogie->x + back_bogie->x) / 2),
        static_cast<int16_t>((front_bogie->y + back_bogie->y) / 2),
        static_cast<int16_t>((front_bogie->z + back_bogie->z) / 2)
    };
    move_to(loc);

    if (object_sprite_type == 0xFF)
        return;

    auto distance_x = front_bogie->x - back_bogie->x;
    auto distance_y = front_bogie->y - back_bogie->y;

    auto offset = sub_4BE368(distance_x * distance_x + distance_y * distance_y);

    auto vehicle_object = object();

    if (vehicle_object->sprites[object_sprite_type].flags & (1 << 4))
    {
        sprite_pitch = vehicle_body_update_sprite_pitch_steep_slopes(offset, front_bogie->z - back_bogie->z);
    }
    else
    {
        sprite_pitch = vehicle_body_update_sprite_pitch(offset, front_bogie->z - back_bogie->z);
    }

    // If the sprite_pitch is odd
    if (sprite_pitch & 1)
    {
        sprite_yaw = vehicle_update_sprite_yaw_1(distance_x, distance_y);
    }
    else
    {
        auto sprite = vehicle_object->sprites[object_sprite_type];
        uint8_t i = sprite_pitch == 0 ? sprite.var_0B : sprite.var_0C;
        switch (i)
        {
            case 0:
                sprite_yaw = vehicle_update_sprite_yaw_0(distance_x, distance_y);
                break;
            case 1:
                sprite_yaw = vehicle_update_sprite_yaw_1(distance_x, distance_y);
                break;
            case 2:
                sprite_yaw = vehicle_update_sprite_yaw_2(distance_x, distance_y);
                break;
            case 3:
                sprite_yaw = vehicle_update_sprite_yaw_3(distance_x, distance_y);
                break;
            case 4:
                sprite_yaw = vehicle_update_sprite_yaw_4(distance_x, distance_y);
                break;
        }
    }
}

// 0x004BE368
uint16_t openloco::vehicle::sub_4BE368(uint32_t distance)
{
    uint8_t i = 10;
    for (; distance > 4096; --i, distance >>= 2)
        ;

    return vehicle_arr_500B50[distance >> 1] >> i;
}

// 0x004BF4DA
uint8_t openloco::vehicle::vehicle_body_update_sprite_pitch_steep_slopes(uint16_t xy_offset, int16_t z_offset)
{
    uint32_t i = 0;

    if (z_offset < 0)
    {
        i = 5;
        z_offset = -z_offset;
    }

    uint32_t xyz = std::numeric_limits<uint32_t>::max();
    if (xy_offset != 0)
    {
        xyz = static_cast<uint64_t>(z_offset << 16) / xy_offset;
    }

    if (xyz > 10064)
    {
        i += 2;
        if (xyz >= 20500)
        {
            i++;
            if (xyz >= 22000)
            {
                i++;
            }
        }
    }
    else
    {
        if (xyz >= 3331)
        {
            i++;
        }
    }

    return vehicleBodyIndexToPitch[i];
}

// 0x004BF49D
uint8_t openloco::vehicle::vehicle_body_update_sprite_pitch(uint16_t xy_offset, int16_t z_offset)
{
    uint32_t i = 0;

    if (z_offset < 0)
    {
        i = 5;
        z_offset = -z_offset;
    }

    uint32_t xyz = std::numeric_limits<uint32_t>::max();
    if (xy_offset != 0)
    {
        xyz = static_cast<uint64_t>(z_offset << 16) / xy_offset;
    }

    if (xyz >= 3331)
    {
        i++;
        if (xyz >= 9000)
        {
            i++;
        }
    }

    return vehicleBodyIndexToPitch[i];
}

// 0x004BF52B
uint8_t openloco::vehicle::vehicle_update_sprite_yaw_0(int16_t x_offset, int16_t y_offset)
{
    uint32_t i = 0;

    if (x_offset < 0)
    {
        i += 2;
        x_offset = -x_offset;
    }

    if (y_offset < 0)
    {
        i += 4;
        y_offset = -y_offset;
    }

    uint32_t xy = std::numeric_limits<uint32_t>::max();
    if (y_offset != 0)
    {
        xy = static_cast<uint64_t>(x_offset << 16) / y_offset;
    }

    if (xy >= 65536)
    {
        i++;
    }

    // 0x00503E66
    constexpr uint8_t indexToYaw[] = {
        16,
        32,
        16,
        0,
        48,
        32,
        48,
        0
    };
    return indexToYaw[i];
}

// 0x004BF5B3
uint8_t openloco::vehicle::vehicle_update_sprite_yaw_1(int16_t x_offset, int16_t y_offset)
{
    uint32_t i = 0;

    if (x_offset < 0)
    {
        i += 3;
        x_offset = -x_offset;
    }

    if (y_offset < 0)
    {
        i += 6;
        y_offset = -y_offset;
    }

    uint32_t xy = std::numeric_limits<uint32_t>::max();
    if (y_offset != 0)
    {
        xy = static_cast<uint64_t>(x_offset << 16) / y_offset;
    }

    if (xy >= 27146)
    {
        i++;
        if (xy >= 158218)
        {
            i++;
        }
    }

    // 0x00503E6E
    constexpr uint8_t indexToYaw[] = {
        16,
        24,
        32,
        16,
        8,
        0,
        48,
        40,
        32,
        48,
        56,
        0
    };
    return indexToYaw[i];
}

// 0x004BF5FB
uint8_t openloco::vehicle::vehicle_update_sprite_yaw_2(int16_t x_offset, int16_t y_offset)
{
    uint32_t i = 0;

    if (x_offset < 0)
    {
        i += 5;
        x_offset = -x_offset;
    }

    if (y_offset < 0)
    {
        i += 10;
        y_offset = -y_offset;
    }

    uint32_t xy = std::numeric_limits<uint32_t>::max();
    if (y_offset != 0)
    {
        xy = static_cast<uint64_t>(x_offset << 16) / y_offset;
    }

    if (xy >= 43790)
    {
        i += 2;
        if (xy >= 98082)
        {
            i++;
            if (xy >= 329472)
            {
                i++;
            }
        }
    }
    else
    {
        if (xy >= 13036)
        {
            i++;
        }
    }

    // 0x00503E7A
    constexpr uint8_t indexToYaw[] = {
        16,
        20,
        24,
        28,
        32,
        16,
        12,
        8,
        4,
        0,
        48,
        44,
        40,
        36,
        32,
        48,
        52,
        56,
        60,
        0
    };
    return indexToYaw[i];
}

// 0x004BF657
uint8_t openloco::vehicle::vehicle_update_sprite_yaw_3(int16_t x_offset, int16_t y_offset)
{
    uint32_t i = 0;

    if (x_offset < 0)
    {
        i += 9;
        x_offset = -x_offset;
    }

    if (y_offset < 0)
    {
        i += 18;
        y_offset = -y_offset;
    }

    uint32_t xy = std::numeric_limits<uint32_t>::max();
    if (y_offset != 0)
    {
        xy = static_cast<uint64_t>(x_offset << 16) / y_offset;
    }

    if (xy >= 79856)
    {
        if (xy >= 216043)
        {
            i += 7;
            if (xy >= 665398)
            {
                i++;
            }
        }
        else
        {
            i += 5;
            if (xy >= 122609)
            {
                i++;
            }
        }
    }
    else
    {
        if (xy >= 19880)
        {
            if (xy >= 35030)
            {
                i += 3;
                if (xy >= 53784)
                {
                    i++;
                }
            }
            else
            {
                i += 2;
            }
        }
        else
        {
            if (xy >= 6455)
            {
                i++;
            }
        }
    }

    // 0x00503E8E
    constexpr uint8_t indexToYaw[] = {
        16,
        18,
        20,
        22,
        24,
        26,
        28,
        30,
        32,
        16,
        14,
        12,
        10,
        8,
        6,
        4,
        2,
        0,
        48,
        46,
        44,
        42,
        40,
        38,
        36,
        34,
        32,
        48,
        50,
        52,
        54,
        56,
        58,
        60,
        62,
        0
    };
    return indexToYaw[i];
}

// 0x004BF6DF
uint8_t openloco::vehicle::vehicle_update_sprite_yaw_4(int16_t x_offset, int16_t y_offset)
{
    uint32_t i = 0;

    if (x_offset < 0)
    {
        i += 17;
        x_offset = -x_offset;
    }

    if (y_offset < 0)
    {
        i += 34;
        y_offset = -y_offset;
    }

    uint32_t xy = std::numeric_limits<uint32_t>::max();
    if (y_offset != 0)
    {
        xy = static_cast<uint64_t>(x_offset << 16) / y_offset;
    }

    if (xy >= 72308)
    {
        if (xy >= 183161)
        {
            if (xy >= 441808)
            {
                i += 15;
                if (xy >= 1334016)
                {
                    i++;
                }
            }
            else
            {
                i += 13;
                if (xy >= 261634)
                {
                    i++;
                }
            }
        }
        else
        {
            if (xy >= 109340)
            {
                i += 11;
                if (xy >= 138564)
                {
                    i++;
                }
            }
            else
            {
                i += 9;
                if (xy >= 88365)
                {
                    i++;
                }
            }
        }
    }
    else
    {
        if (xy >= 23449)
        {
            if (xy >= 39281)
            {
                i += 6;
                if (xy >= 48605)
                {
                    i++;
                    if (xy >= 59398)
                    {
                        i++;
                    }
                }
            }
            else
            {
                i += 4;
                if (xy >= 30996)
                {
                    i++;
                }
            }
        }
        else
        {
            if (xy >= 9721)
            {
                i += 2;
                if (xy >= 16416)
                {
                    i++;
                }
            }
            else
            {
                if (xy >= 3220)
                {
                    i++;
                }
            }
        }
    }

    // 0x00503EB2
    constexpr uint8_t indexToYaw[] = {
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
        32,
        16,
        15,
        14,
        13,
        12,
        11,
        10,
        9,
        8,
        7,
        6,
        5,
        4,
        3,
        2,
        1,
        0,
        48,
        47,
        46,
        45,
        44,
        43,
        42,
        41,
        40,
        39,
        38,
        37,
        36,
        35,
        34,
        33,
        32,
        48,
        49,
        50,
        51,
        52,
        53,
        54,
        55,
        56,
        57,
        58,
        59,
        60,
        61,
        62,
        63,
        0
    };
    return indexToYaw[i];
}

// 0x004AB655
void openloco::vehicle::secondary_animation_update()
{
    auto vehicleObject = object();
    uint8_t var_05 = vehicleObject->var_24[var_54].var_05;
    if (var_05 == 0)
        return;

    var_05 -= 0x80;

    switch (vehicleObject->animation[1].type)
    {
        case simple_animation_type::none:
            return;
        case simple_animation_type::steam_puff1:
        case simple_animation_type::steam_puff2:
        case simple_animation_type::steam_puff3:
            steam_puffs_animation_update(1, var_05);
            break;
        case simple_animation_type::diesel_exhaust1:
            diesel_exhaust1_animation_update(1, var_05);
            break;
        case simple_animation_type::electric_spark1:
            electric_spark1_animation_update(1, var_05);
            break;
        case simple_animation_type::electric_spark2:
            electric_spark2_animation_update(1, var_05);
            break;
        case simple_animation_type::diesel_exhaust2:
            diesel_exhaust2_animation_update(1, var_05);
            break;
        case simple_animation_type::ship_wake:
            ship_wake_animation_update(1, var_05);
            break;
        default:
            assert(false);
            break;
    }
}

// 0x004AB688, 0x004AACA5
void openloco::vehicle::steam_puffs_animation_update(uint8_t num, int8_t var_05)
{
    auto vehicleObject = object();
    vehicle* frontBogie = vehicle_front_bogie;
    vehicle* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* vehType2 = vehicle_1136120;
    bool soundCode = false;
    if (vehType2->var_5A == 1 || vehType2->var_5A == 4)
    {
        soundCode = true;
    }
    bool tickCalc = true;
    if (vehType2->var_5A != 0 && vehType2->var_56 >= 65536)
    {
        tickCalc = false;
    }

    auto _var_44 = var_44;
    // Reversing
    if (var_38 & (1 << 1))
    {
        var_05 = -var_05;
        _var_44 = -_var_44;
    }

    if (tickCalc && (soundCode == false))
    {
        if (scenario_ticks() & 7)
            return;
    }
    else
    {
        if (vehicle_var_1136130 + (uint16_t)(_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        {
            return;
        }
    }

    var_05 += 64;
    loc16 loc = {
        static_cast<int16_t>(backBogie->x - frontBogie->x),
        static_cast<int16_t>(backBogie->y - frontBogie->y),
        static_cast<int16_t>(backBogie->z - frontBogie->z),
    };

    loc.x = loc.x * var_05 / 128;
    loc.y = loc.y * var_05 / 128;
    loc.z = loc.z * var_05 / 128;

    loc.x += frontBogie->x;
    loc.y += frontBogie->y;
    loc.z += frontBogie->z;

    loc.z += vehicleObject->animation[num].height;

    auto xyFactor = vehicleObject->animation[num].height * factor503B50[sprite_pitch];
    xyFactor /= 256;

    auto xFactor = xyFactor * factorXY503B6A[sprite_yaw * 2];
    auto yFactor = xyFactor * factorXY503B6A[sprite_yaw * 2 + 1];

    xFactor /= 256;
    yFactor /= 256;

    loc.x += xFactor;
    loc.y += yFactor;

    exhaust::create(loc, vehicleObject->animation[num].object_id | (soundCode ? 0 : 0x80));
    if (soundCode == false)
        return;

    var_55++;
    steam_object* steam_obj = objectmgr::get<steam_object>(vehicleObject->animation[num].object_id);
    if (var_55 >= ((uint8_t)vehicleObject->animation[num].type) + 1)
    {
        var_55 = 0;
    }

    bool itemFound = false;

    // Looking for a bridge? or somthing ontop
    if (steam_obj->var_08 & (1 << 2))
    {
        auto tile = map::tilemgr::get(frontBogie->tile_x, frontBogie->tile_y);

        for (auto& el : tile)
        {
            if (itemFound && !(el.flags() & ((1 << 5) | (1 << 4))))
            {
                break;
            }
            else
            {
                itemFound = false;
            }
            auto elUnk1 = el.as_unk1();
            if (elUnk1 == nullptr)
                continue;
            if (elUnk1->base_z() != frontBogie->tile_base_z)
                continue;
            if (elUnk1->unk_z() != loc.z)
                continue;

            if (!elUnk1->has_80())
                continue;

            if (!elUnk1->is_last())
                itemFound = true;
        }
    }

    if (itemFound)
    {
        auto soundId = static_cast<audio::sound_id>(steam_obj->var_1F[var_55 + (steam_obj->sound_effect >> 1)]);

        if (vehType2->var_56 > 983040)
            return;

        int32_t volume = 0 - (vehType2->var_56 >> 9);

        auto height = std::get<0>(map::tilemgr::get_height(loc.x, loc.y));

        if (loc.z <= height)
        {
            volume -= 1500;
        }

        audio::play_sound(audio::make_object_sound_id(soundId), loc, volume, 22050);
    }
    else
    {
        auto soundModifier = steam_obj->sound_effect >> 1;
        if (!(steam_obj->var_08 & (1 << 2)))
        {
            soundModifier = 0;
        }
        auto underSoundId = static_cast<audio::sound_id>(steam_obj->var_1F[soundModifier + var_55]);
        auto soundId = static_cast<audio::sound_id>(steam_obj->var_1F[var_55]);

        if (vehType2->var_56 > 983040)
            return;

        int32_t volume = 0 - (vehType2->var_56 >> 9);

        auto height = std::get<0>(map::tilemgr::get_height(loc.x, loc.y));

        if (loc.z <= height)
        {
            soundId = underSoundId;
            volume -= 1500;
        }

        if (volume > -400)
        {
            volume = -400;
        }

        audio::play_sound(audio::make_object_sound_id(soundId), loc, volume, 22050);
    }
}

// 0x004AB9DD & 0x004AAFFA
void openloco::vehicle::diesel_exhaust1_animation_update(uint8_t num, int8_t var_05)
{
    vehicle* frontBogie = vehicle_front_bogie;
    vehicle* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle* veh = vehicle_1136118;
    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (veh->var_5E == 5)
    {
        if (vehType2->var_56 == 0)
            return;

        if (var_38 & (1 << 1))
        {
            var_05 = -var_05;
        }

        if (scenario_ticks() & 3)
            return;

        auto positionFactor = vehicleObject->sprites[0].bogey_position * var_05 / 256;
        auto invertedDirection = sprite_yaw ^ (1 << 5);
        auto xFactor = (factorXY503B6A[invertedDirection * 2] * positionFactor) / 512;
        auto yFactor = (factorXY503B6A[invertedDirection * 2 + 1] * positionFactor) / 512;

        loc16 loc = {
            static_cast<int16_t>(x + xFactor),
            static_cast<int16_t>(y + yFactor),
            static_cast<int16_t>(z + vehicleObject->animation[num].height)
        };
        exhaust::create(loc, vehicleObject->animation[num].object_id);
    }
    else
    {
        if (vehType2->var_5A != 1)
            return;

        if (var_38 & (1 << 1))
        {
            var_05 = -var_05;
        }

        if (scenario_ticks() & 3)
            return;

        if (var_5E != 0)
            return;

        var_05 += 64;
        loc16 loc = {
            static_cast<int16_t>(backBogie->x - frontBogie->x),
            static_cast<int16_t>(backBogie->y - frontBogie->y),
            static_cast<int16_t>(backBogie->z - frontBogie->z),
        };

        loc.x = loc.x * var_05 / 128;
        loc.y = loc.y * var_05 / 128;
        loc.z = loc.z * var_05 / 128;

        loc.x += frontBogie->x;
        loc.y += frontBogie->y;
        loc.z += frontBogie->z;

        loc.z += vehicleObject->animation[num].height;

        auto xyFactor = vehicleObject->animation[num].height * factor503B50[sprite_pitch];
        xyFactor /= 256;

        auto xFactor = xyFactor * factorXY503B6A[sprite_yaw * 2];
        auto yFactor = xyFactor * factorXY503B6A[sprite_yaw * 2 + 1];

        xFactor /= 256;
        yFactor /= 256;

        loc.x += xFactor;
        loc.y += yFactor;

        exhaust::create(loc, vehicleObject->animation[num].object_id);
    }
}

// 0x004ABB5A & 0x004AB177
void openloco::vehicle::diesel_exhaust2_animation_update(uint8_t num, int8_t var_05)
{
    vehicle* frontBogie = vehicle_front_bogie;
    vehicle* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (vehType2->var_5A != 1)
        return;

    if (vehType2->var_56 > 917504)
        return;

    if (var_38 & (1 << 1))
    {
        var_05 = -var_05;
    }

    if (scenario_ticks() & 7)
        return;

    var_05 += 64;

    loc16 loc = {
        static_cast<int16_t>(backBogie->x - frontBogie->x),
        static_cast<int16_t>(backBogie->y - frontBogie->y),
        static_cast<int16_t>(backBogie->z - frontBogie->z),
    };

    loc.x = loc.x * var_05 / 128;
    loc.y = loc.y * var_05 / 128;
    loc.z = loc.z * var_05 / 128;

    loc.x += frontBogie->x;
    loc.y += frontBogie->y;
    loc.z += frontBogie->z;

    loc.z += vehicleObject->animation[num].height;

    auto xyFactor = vehicleObject->animation[num].height * factor503B50[sprite_pitch];
    xyFactor /= 256;

    auto xFactor = xyFactor * factorXY503B6A[sprite_yaw * 2];
    auto yFactor = xyFactor * factorXY503B6A[sprite_yaw * 2 + 1];

    xFactor /= 256;
    yFactor /= 256;

    loc.x += xFactor;
    loc.y += yFactor;

    auto yaw = (sprite_yaw + 16) & 0x3F;

    xyFactor = 5;
    if (vehicle_var_525FAE != 0)
    {
        xyFactor = -5;
    }

    xFactor = xyFactor * factorXY503B6A[yaw * 2];
    yFactor = xyFactor * factorXY503B6A[yaw * 2 + 1];

    xFactor /= 256;
    yFactor /= 256;

    loc.x += xFactor;
    loc.y += yFactor;

    exhaust::create(loc, vehicleObject->animation[num].object_id);
}

// 0x004ABDAD & 0x004AB3CA
void openloco::vehicle::electric_spark1_animation_update(uint8_t num, int8_t var_05)
{
    vehicle* frontBogie = vehicle_front_bogie;
    vehicle* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (vehType2->var_5A != 2 && vehType2->var_5A != 1)
        return;

    auto _var_44 = var_44;
    if (var_38 & (1 << 1))
    {
        var_05 = -var_05;
        _var_44 = -var_44;
    }

    if (((uint16_t)vehicle_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        return;

    var_05 += 64;

    if (gprng().rand_next(std::numeric_limits<uint16_t>::max()) > 819)
        return;

    loc16 loc = {
        static_cast<int16_t>(backBogie->x - frontBogie->x),
        static_cast<int16_t>(backBogie->y - frontBogie->y),
        static_cast<int16_t>(backBogie->z - frontBogie->z),
    };

    loc.x = loc.x * var_05 / 128;
    loc.y = loc.y * var_05 / 128;
    loc.z = loc.z * var_05 / 128;

    loc.x += frontBogie->x;
    loc.y += frontBogie->y;
    loc.z += frontBogie->z;

    loc.z += vehicleObject->animation[num].height;

    auto xyFactor = vehicleObject->animation[num].height * factor503B50[sprite_pitch];
    xyFactor /= 256;

    auto xFactor = xyFactor * factorXY503B6A[sprite_yaw * 2];
    auto yFactor = xyFactor * factorXY503B6A[sprite_yaw * 2 + 1];

    xFactor /= 256;
    yFactor /= 256;

    loc.x += xFactor;
    loc.y += yFactor;

    exhaust::create(loc, vehicleObject->animation[num].object_id);
}

// 0x004ABEC3 & 0x004AB4E0
void openloco::vehicle::electric_spark2_animation_update(uint8_t num, int8_t var_05)
{
    vehicle* frontBogie = vehicle_front_bogie;
    vehicle* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (vehType2->var_5A != 2 && vehType2->var_5A != 1)
        return;

    auto _var_44 = var_44;
    if (var_38 & (1 << 1))
    {
        var_05 = -var_05;
        _var_44 = -var_44;
    }

    if (((uint16_t)vehicle_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        return;

    var_05 += 64;

    if (gprng().rand_next(std::numeric_limits<uint16_t>::max()) > 936)
        return;

    loc16 loc = {
        static_cast<int16_t>(backBogie->x - frontBogie->x),
        static_cast<int16_t>(backBogie->y - frontBogie->y),
        static_cast<int16_t>(backBogie->z - frontBogie->z),
    };

    loc.x = loc.x * var_05 / 128;
    loc.y = loc.y * var_05 / 128;
    loc.z = loc.z * var_05 / 128;

    loc.x += frontBogie->x;
    loc.y += frontBogie->y;
    loc.z += frontBogie->z;

    loc.z += vehicleObject->animation[num].height;

    auto xyFactor = vehicleObject->animation[num].height * factor503B50[sprite_pitch];
    xyFactor /= 256;

    auto xFactor = xyFactor * factorXY503B6A[sprite_yaw * 2];
    auto yFactor = xyFactor * factorXY503B6A[sprite_yaw * 2 + 1];

    xFactor /= 256;
    yFactor /= 256;

    loc.x += xFactor;
    loc.y += yFactor;

    auto yaw = (sprite_yaw + 16) & 0x3F;
    auto firstBogie = var_38 & (1 << 1) ? backBogie : frontBogie;
    xyFactor = 5;
    if (!(vehicle_arr_4F8A7C[firstBogie->var_2C / 8] & 1))
    {
        xyFactor = -5;
    }

    if (firstBogie->var_2C & (1 << 2))
    {
        xyFactor = -xyFactor;
    }

    xFactor = xyFactor * factorXY503B6A[yaw * 2];
    yFactor = xyFactor * factorXY503B6A[yaw * 2 + 1];

    xFactor /= 256;
    yFactor /= 256;

    loc.x += xFactor;
    loc.y += yFactor;

    exhaust::create(loc, vehicleObject->animation[num].object_id);
}

// 0x004ABC8A & 0x004AB2A7
void openloco::vehicle::ship_wake_animation_update(uint8_t num, int8_t var_05)
{
    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (vehType2->var_5A == 0)
        return;

    if (vehType2->var_56 < 393216)
        return;

    auto frequency = 32;
    if (vehType2->var_56 >= 589824)
    {
        frequency = 16;
        if (vehType2->var_56 >= 851968)
        {
            frequency = 8;
            if (vehType2->var_56 >= 1638400)
            {
                frequency = 4;
            }
        }
    }

    if ((scenario_ticks() % frequency) != 0)
        return;

    auto positionFactor = vehicleObject->sprites[0].bogey_position;
    auto invertedDirection = sprite_yaw ^ (1 << 5);
    auto xFactor = (factorXY503B6A[invertedDirection * 2] * positionFactor) / 1024;
    auto yFactor = (factorXY503B6A[invertedDirection * 2 + 1] * positionFactor) / 1024;

    loc16 loc = {
        static_cast<int16_t>(x + xFactor),
        static_cast<int16_t>(y + yFactor),
        z
    };

    auto yaw = (sprite_yaw + 16) & 0x3F;

    xFactor = vehicleObject->var_113 * factorXY503B6A[yaw * 2];
    yFactor = vehicleObject->var_113 * factorXY503B6A[yaw * 2 + 1];

    xFactor /= 512;
    yFactor /= 512;

    loc.x += xFactor;
    loc.y += yFactor;

    exhaust::create(loc, vehicleObject->animation[num].object_id);

    if (vehicleObject->var_113 == 0)
        return;

    yaw = (sprite_yaw - 16) & 0x3F;

    xFactor = vehicleObject->var_113 * factorXY503B6A[yaw * 2];
    yFactor = vehicleObject->var_113 * factorXY503B6A[yaw * 2 + 1];

    xFactor /= 512;
    yFactor /= 512;

    loc.x += xFactor;
    loc.y += yFactor;

    exhaust::create(loc, vehicleObject->animation[num].object_id);
}

void vehicle::sub_4AF06E()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004AF06E, regs);
}
