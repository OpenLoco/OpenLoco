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
loco_global<vehicle*, 0x01136124> vehicle_front_bogie;
loco_global<vehicle*, 0x01136128> vehicle_back_bogie;
loco_global<int32_t, 0x01136130> vehicle_var_1136130;
loco_global<vehicle*, 0x01136120> vehicle_1136120;
loco_global<uint8_t, 0x01136237> vehicle_var_1136237;         // var_28 related?
loco_global<uint8_t, 0x01136238> vehicle_var_1136238;         // var_28 related?
loco_global_array<int8_t, 88, 0x004F865C> vehicle_arr_4F865C; // var_2C related?
loco_global_array<uint16_t, 2047, 0x00500B50> vehicle_arr_500B50;
loco_global_array<int16_t, 128, 0x00503B6A> factorXY503B6A;

// 0x00503E5C
static constexpr uint8_t vehicleBodyIndexToPitch[] =
{
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
        sub_4AC255(vehicle_back_bogie, vehicle_front_bogie);
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
        // Just returns??
        sub_4AB655(vehicleObject, vehicleObject->var_24[var_54].var_05);
        return;
    }

    regs.ebx -= 0x80;
    switch (vehicleObject->vis_fx_type)
    {
        case 0:
            sub_4AB655(vehicleObject, vehicleObject->var_24[var_54].var_05);
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
        vehicle* frontBogie = vehicle_front_bogie;
        vehicle* veh3 = vehicle_1136120;
        al = var_46;
        int8_t ah = 0;
        if (veh3->var_56 < 0x230000)
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

// 0x004AC255
void openloco::vehicle::sub_4AC255(vehicle * back_bogie, vehicle * front_bogie)
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
        sprite_pitch = vehicle_body_update_sprite_pitch_special(offset, front_bogie->z - back_bogie->z);
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
    for (; distance > 4096; --i, distance >>= 2);

    return vehicle_arr_500B50[distance >> 1] >> i;
}

// 0x004BF4DA
uint8_t openloco::vehicle::vehicle_body_update_sprite_pitch_special(uint16_t xy_offset, int16_t z_offset)
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
    constexpr uint8_t indexToYaw[] =
    {
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
    constexpr uint8_t indexToYaw[] =
    {
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
    constexpr uint8_t indexToYaw[] =
    {
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
    constexpr uint8_t indexToYaw[] =
    {
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
    constexpr uint8_t indexToYaw[] =
    {
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
void openloco::vehicle::sub_4AB655(vehicle_object * veh_object, uint8_t var_05)
{
    if (var_05 == 0)
        return;

    var_05 -= 0x80;

    switch (veh_object->wake_fx_type)
    {
    case 0:
        return;
    case 1:
    case 2:
    case 3:
        sub_4AB688(veh_object, var_05);
        break;
    case 4:
        // 0x004AB9DD
        //break;
    case 5:
        // 0x004ABDAD
        //break;
    case 6:
        // 0x004ABEC3
        //break;
    case 7:
        // 0x004ABB5A
        //break;
    case 8:
        // 0x004ABC8A
        break;
    default:
        registers regs;
        regs.esi = (int32_t)this;
        regs.ebp = (int32_t)veh_object;
        regs.ebx = var_05 + 0x80;
        call(0x004AB655, regs);
        break;
    }
}

// 0x004AB688
void openloco::vehicle::sub_4AB688(vehicle_object * veh_object, uint8_t var_05)
{
    vehicle * frontBogie = vehicle_front_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle * veh_3 = vehicle_1136120;
    if (veh_3->var_5A != 1 && veh_3->var_5A != 4)
    {
        bool tickCalc = true;
        if (veh_3->var_5A != 0 && veh_3->var_56 >= 65536)
        {
            tickCalc = false;
        }

        auto _var_44 = var_44;
        if (var_38 & (1 << 1))
        {
            var_05 = -var_05;
            _var_44 = -_var_44;
        }

        if (tickCalc)
        {
            if (scenario_ticks() & 7)
                return;
        }
        else
        {
            if ((vehicle_var_1136130 + _var_44 * 8) < std::numeric_limits<uint16_t>::max())
            {
                return;
            }
        }

        var_05 += 64;
        vehicle * backBogie = vehicle_back_bogie;
        loc16 loc = {
            backBogie->x - frontBogie->x,
            backBogie->y - frontBogie->y,
            backBogie->z - frontBogie->z,
        };

        if (loc.x != 0)
        {
            loc.x = loc.x * var_05 / 128;
        }

        if (loc.y != 0)
        {
            loc.y = loc.y * var_05 / 128;
        }

        if (loc.z != 0)
        {
            loc.z = loc.z * var_05 / 128;
        }

        loc.x += frontBogie->x;
        loc.y += frontBogie->y;
        loc.z += frontBogie->z;


        loc.z += veh_object->var_111;

        auto xyFactor = veh_object->var_111 * factor503B50[sprite_pitch];
        if (xyFactor != 0)
        {
            xyFactor /= 256;
        }

        auto xFactor = xyFactor * factorXY503B6A[sprite_yaw * 2];
        auto yFactor = xyFactor * factorXY503B6A[sprite_yaw * 2 + 1];

        if (xFactor != 0)
        {
            xFactor /= 256;
        }

        if (yFactor != 0)
        {
            yFactor /= 256;
        }

        loc.x += xFactor;
        loc.y += yFactor;

        registers regs;

        regs.bl = veh_object->var_110 | 0x80;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        call(0x0044080C, regs);

        return;
    }

    registers reg;
    reg.esi = (int32_t)this;
    reg.ebp = (int32_t)veh_object;
    reg.ebx = var_05;
    call(0x004AB7A8, reg);
}
