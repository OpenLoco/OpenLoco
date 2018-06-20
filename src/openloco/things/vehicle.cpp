#include "vehicle.h"
#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../map/tilemgr.h"
#include "../messagemgr.h"
#include "../objects/airport_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/vehicle_object.h"
#include "../openloco.h"
#include "../stationmgr.h"
#include "../utility/numeric.hpp"
#include "../viewportmgr.h"
#include "../windowmgr.h"
#include "misc.h"
#include "thingmgr.h"
#include <algorithm>
#include <cassert>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::objectmgr;
using namespace openloco::literals;

loco_global<uint8_t[2], 0x0113601A> vehicle_var_113601A;
#pragma pack(push, 1)
struct array_113609C
{
    uint32_t size;      // 0x0113609C
    uint16_t data[22];   // 0x011360A0
};
#pragma pack(pop)

loco_global<array_113609C, 0x0113609C> vehicle_var_array_11360A0;
loco_global<uint32_t, 0x0113609C> vehicle_var_113609C; // Don't use. use vehicle_var_array_11360A0
loco_global<uint16_t, 0x011360A0> vehicle_var_11360A0; // Don't use. use vehicle_var_array_11360A0
loco_global<uint32_t, 0x011360D0> vehicle_var_manhattan_distance_to_station;
loco_global<uint16_t, 0x01136168> vehicle_var_target_z; // no longer used
loco_global<uint8_t, 0x0113646D> vehicle_var_113646D;
loco_global<vehicle_0*, 0x01136118> vehicle_1136118; // type 0
loco_global<vehicle_1*, 0x0113611C> vehicle_113611C; // type 1
loco_global<vehicle_2*, 0x01136120> vehicle_1136120; // type 2
loco_global<vehicle_bogie*, 0x01136124> vehicle_front_bogie;
loco_global<vehicle_bogie*, 0x01136128> vehicle_back_bogie;
loco_global<int32_t, 0x0113612C> vehicle_var_113612C;
loco_global<int32_t, 0x01136130> vehicle_var_1136130;
loco_global<uint8_t, 0x01136237> vehicle_var_1136237;   // var_28 related?
loco_global<uint8_t, 0x01136238> vehicle_var_1136238;   // var_28 related?
loco_global<uint8_t, 0x0113646C> vehicle_var_113646C;   // var_5D related? used only in 4B980A
loco_global<int8_t[88], 0x004F865C> vehicle_arr_4F865C; // var_2C related?
loco_global<uint16_t[2047], 0x00500B50> vehicle_arr_500B50;
loco_global<int16_t[128], 0x00503B6A> factorXY503B6A;
loco_global<uint8_t[44], 0x004F8A7C> vehicle_arr_4F8A7C;   // bools
loco_global<uint32_t, 0x00525BB0> vehicle_var_525BB0;      // airport_object_var_AE_flags
loco_global<uint8_t, 0x00525FAE> vehicle_var_525FAE;       // boolean
loco_global<uint8_t[128000], 0x987C5C> vehicle_var_987C5C; // Size tbc
loco_global<uint32_t[7], 0x004FE070> vehicle_var_4FE070;   // Size tbc
loco_global<uint8_t[7], 0x004FE088> vehicle_var_4FE088;    // Size tbc
loco_global<station_id_t, 0x01135FAE> vehicle_var_1135FAE;
loco_global<uint8_t, 0x0113607D> vehicle_var_113607D;  // bool
loco_global<uint16_t *, 0x01135EE6> vehicle_var_1135EE6; // vehicle_var_11360A0 related?
#pragma pack(push, 1)
struct unk_4F73D8
{
    uint8_t pad_00[0x5 - 0x00];
    int16_t var_05;
    uint8_t pad_07[0xA - 0x7];
};
#pragma pack(pop)
loco_global<unk_4F73D8*[44], 0x004F73D8> vehicle_var_4F73D8;
#pragma pack(push, 1)
struct unk_4F7B5C
{
    uint8_t unk_0;
    uint8_t unk_1;
    int16_t x;
    int16_t y;
    int16_t z;
};
#pragma pack(pop)
loco_global<unk_4F7B5C[352], 0x004F7B5C> vehicle_var_4F7B5C;  // Size tbc 0x160

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

vehicle* vehicle_base::next_vehicle()
{
    return thingmgr::get<vehicle>(((vehicle*)this)->next_thing_id);
}

vehicle* vehicle_base::next_car()
{
    return thingmgr::get<vehicle>(((vehicle*)this)->next_car_id);
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

vehicle_object* vehicle_body::object() const
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
        case vehicle_thing_type::vehicle_0:
        {
            auto veh = this->as_vehicle_0();
            if (veh != nullptr)
            {
                return !veh->Update();
            }
            break;
        }
        case vehicle_thing_type::vehicle_1:
            result = call(0x004A9788, regs);
            break;
        case vehicle_thing_type::vehicle_2:
            result = call(0x004A9B0B, regs);
            break;
        case vehicle_thing_type::vehicle_bogie:
            result = call(0x004AA008, regs);
            break;
        case vehicle_thing_type::vehicle_body_end:
        case vehicle_thing_type::vehicle_body_cont:
        {
            auto veh = this->as_vehicle_body();
            if (veh != nullptr)
            {
                return !veh->Update();
            }
            break;
        }
        case vehicle_thing_type::vehicle_6:
            result = call(0x004AA24A, regs);
            break;
        default:
            break;
    }
    return (result & (1 << 8)) != 0;
}

// 0x004BA8D4
void vehicle_0::sub_4BA8D4()
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
    if (v->type != vehicle_thing_type::vehicle_6)
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
                if (v->type == vehicle_thing_type::vehicle_6)
                {
                    return;
                }
                u = v->next_car()->next_car();
                if (u->type != vehicle_thing_type::vehicle_body_end)
                    v = u->next_car();
            } while (u->type != vehicle_thing_type::vehicle_body_end);
        }
    }
}

void vehicle_0::sub_4BAA76()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004BAA76, regs);
}

// 0x004A8B81
bool openloco::vehicle_0::Update()
{
    vehicle_1136118 = this;
    auto veh = next_car();
    vehicle_113611C = veh->as_vehicle_1();
    veh = veh->next_car();
    vehicle_1136120 = veh->as_vehicle_2();

    vehicle_var_113646C = var_5D;
    sub_4A8882();

    vehicle_front_bogie = (vehicle_bogie*)0xFFFFFFFF;
    vehicle_back_bogie = (vehicle_bogie*)0xFFFFFFFF;

    vehicle_2* vehType2 = vehicle_1136120;
    vehicle_var_113612C = vehType2->speed >> 7;
    vehicle_var_1136130 = vehType2->speed >> 7;

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
            return false;
        }

        vehType6->var_4F++;
        if (vehType6->var_4F < 960)
        {
            return false;
        }
        sub_4AF06E();
        return false;
    }
    sub_4BA8D4();

    if (v_class == vehicle_class::air)
    {
        return update_plane();
    }
    else if (v_class == vehicle_class::water)
    {
        return update_boat();
    }
    else
    {
        return update_other();
    }
}

// 0x004A8C11
bool openloco::vehicle_0::update_other()
{
    vehicle_2* vehType2 = vehicle_1136120;
    if ((!(vehType2->var_73 & flags_73::broken_down) || (vehType2->var_73 & flags_73::unk_2)) && var_5D == 4)
    {
        if (v_class == vehicle_class::road)
        {
            uint8_t bl = sub_4AA36A();
            if (bl == 1)
            {
                return sub_4A8DB7();
            }
            else if (bl == 2)
            {
                return sub_4A8F22();
            }
        }

        if (var_0C & (1 << 1))
        {
            return sub_4A8CB6();
        }
        else if (var_0C & (1 << 6))
        {
            if (var_6E <= 236)
            {
                return sub_4A8C81();
            }
        }

        return sub_4A8FAC();
    }
    else
    {
        if (var_5D == 5)
        {
            sub_4B9A2A();

            sub_4B980A();
            return true;
        }
        else if (var_5D == 6)
        {
            return sub_4A9011();
        }
        else if (var_5D == 8)
        {
            sub_4AA625();

            return false;
        }
        else if (var_5D == 9)
        {
            return false;
        }
        else
        {
            var_5D = 2;

            if (!(vehType2->var_73 & flags_73::broken_down) || (vehType2->var_73 & flags_73::unk_2))
            {
                if (!(var_0C & (1 << 6)) || var_6E > 236)
                {
                    if (!(var_0C & (1 << 1)))
                    {
                        return sub_4A8D48();
                    }
                    else
                    {
                        return sub_4A8CB6();
                    }
                }
                else
                {
                    return sub_4A8C81();
                }
            }
            else
            {
                return sub_4A8CB6();
            }
        }
    }
}

// 0x004A8CB6
bool openloco::vehicle_0::sub_4A8CB6()
{
    vehicle_1* vehType1 = vehicle_113611C;

    if (x != vehType1->x || y != vehType1->y || z != vehType1->z)
    {
        sub_4AD93A();
        if (var_5D == 4)
        {
            station_id = 0xFFFF;
            var_5D = 2;
        }
    }

    auto vehUnk = next_car()->next_car();
    if (vehUnk->var_36 != var_36)
    {
        sub_4B980A();
        return true;
    }

    var_5D = 1;
    vehicle_2* vehType2 = vehicle_1136120;

    if (!(vehType2->var_73 & flags_73::broken_down))
    {
        sub_4B980A();
        return true;
    }

    station_id = 0xFFFF;
    var_5D = 7;

    sub_4B980A();
    return true;
}

// 0x004A8C81
bool openloco::vehicle_0::sub_4A8C81()
{
    vehicle_2* vehType2 = vehicle_1136120;
    if (vehType2->speed > 1_mph32)
    {
        return sub_4A8D48();
    }

    station_id = sub_4BABAD();
    sub_4B996F();
    sub_4B9987();
    sub_4BACAF();
    sub_4B99E1();

    return sub_4A8CB6();
}

// 0x004A8D48
bool openloco::vehicle_0::sub_4A8D48()
{
    sub_4707C0();
    uint8_t al = 0, ah = 0;
    uint16_t stationId = 0;
    sub_4ACEE7(13945600, vehicle_var_113612C, al, ah, stationId);

    if (v_class == vehicle_class::road)
    {
        uint8_t bl = sub_4AA36A();
        if (bl == 1)
        {
            return sub_4A8DB7();
        }
        else if (bl == 2)
        {
            return sub_4A8F22();
        }
        else if (al == 4)
        {
            var_5D = 4;
            station_id = stationId;
            sub_4B980A();
            return true;
        }
        else if (al == 2)
        {
            auto veh = next_car()->next_car()->as_vehicle_2();
            if (veh == nullptr)
            {
                assert(false);
                // Todo fail gracefully
                return false;
            }
            if (var_36 != veh->var_36 || veh->var_2E != var_2E)
            {
                sub_4B980A();
                return true;
            }
            return sub_4A8F22();
        }
        else
        {
            sub_4B980A();
            return true;
        }
    }
    else
    {
        if (al == 4)
        {
            var_5D = 4;
            station_id = stationId;
            sub_4B980A();
            return true;
        }
        else if (al == 3)
        {
            auto veh = next_car()->next_car()->as_vehicle_2();
            if (veh == nullptr)
            {
                assert(false);
                // Todo fail gracefully
                return false;
            }
            if (veh->var_36 != var_36 || veh->var_2E != var_2E)
            {
                sub_4B980A();
                return true;
            }

            var_5D = 3;
            auto vehType1 = next_car()->as_vehicle_1();
            if (vehType1 == nullptr)
            {
                assert(false);
                // Todo fail gracefully
                return false;
            }
            vehType1->var_46++;

            if (var_0C & (1 << 6))
            {
                var_5C = 2;
                vehType1->var_48 |= 1 << 0;
                sub_4B980A();
                return true;
            }

            if (ah & (1 << 1))
            {
                if (vehType1->var_46 < 1920)
                {
                    sub_4B980A();
                    return true;
                }

                if (!(ah & (1 << 7)))
                {
                    if (sub_4AC1C2())
                    {
                        var_5C = 2;
                        vehType1->var_48 |= 1 << 0;
                        sub_4B980A();
                        return true;
                    }
                }
                return sub_4A8ED9();
            }
            else
            {
                if (!(vehType1->var_46 & 0x3F))
                {
                    if (!(ah & (1 << 7)))
                    {
                        if (sub_4AC1C2())
                        {
                            var_5C = 2;
                            vehType1->var_48 |= 1 << 0;
                            sub_4B980A();
                            return true;
                        }
                    }

                    if (sub_4AC0A3())
                    {
                        return sub_4A8ED9();
                    }
                }

                if (vehType1->var_46 < 640)
                {
                    sub_4B980A();
                    return true;
                }
                return sub_4A8ED9();
            }
        }
        else
        {
            auto veh = next_car()->as_vehicle_1();
            if (veh == nullptr)
            {
                assert(false);
                // Todo fail gracefully
                return false;
            }
            veh->var_46 = 0;
            if (al == 2)
            {
                if (!(var_0C & (1 << 6)))
                {
                    return sub_4A8ED9();
                }

                vehicle_2* vehType2 = veh->next_car()->as_vehicle_2();
                if (vehType2 == nullptr)
                {
                    assert(false);
                    // Todo fail gracefully
                    return false;
                }
                if (vehType2->var_36 != var_36 || vehType2->var_2E != var_2E)
                {
                    return sub_4A8ED9();
                }

                vehType2->sub_4AA464();
                return false;
            }
            else
            {
                sub_4B980A();
                return true;
            }
        }
    }
}

// 0x004A8DB7
bool openloco::vehicle_0::sub_4A8DB7()
{
    sub_4AD778();
    if (var_5D == 4)
    {
        var_5D = 2;
    }
    sub_4B980A();
    return true;
}

// 0x004A8F22
bool openloco::vehicle_0::sub_4A8F22()
{
    if (sub_4BADE4())
    {
        auto temp = var_52;
        var_52 = 1;
        sub_4ADB47(0);
        var_52 = temp;
        sub_4B980A();
        return true;
    }
    else
    {
        auto veh = next_car()->next_car()->as_vehicle_2();
        veh->sub_4AA464();
        return false;
    }
}

// 0x004A8ED9
bool openloco::vehicle_0::sub_4A8ED9()
{
    vehicle* veh = next_car();
    veh->var_46 = 0;

    veh = veh->next_car();
    if (var_36 != veh->var_36 || veh->var_2E != var_2E)
    {
        sub_4B980A();
        return true;
    }
    return sub_4A8F22();
}

// 0x004A9011
bool openloco::vehicle_0::sub_4A9011()
{
    if (sub_4BA142())
    {
        sub_4B980A();
        return true;
    }

    sub_4BAC74();
    var_5D = 1;
    sub_4707C0();

    if (var_0C & (1 << 6))
    {
        sub_4B980A();
        return true;
    }

    if (sub_4ACCDC())
    {
        return sub_4A8F22();
    }

    sub_4B980A();
    return true;
}

// 0x004A8FAC
bool openloco::vehicle_0::sub_4A8FAC()
{
    vehicle* veh = next_car()->next_car();
    if (var_36 != veh->var_36 || veh->var_2E != var_2E)
    {
        sub_4B980A();
        return true;
    }

    if (var_0C & (1 << 6))
    {
        sub_4B980A();
        return true;
    }

    sub_4B996F();
    sub_4B9987();
    sub_4BACAF();
    sub_4B99E1();

    sub_4B980A();
    return true;
}

// 0x004A9051
bool openloco::vehicle_0::update_plane()
{
    vehicle_var_1136130 = 8192;
    vehicle_2* vehType2 = vehicle_1136120;

    if (vehType2->speed >= 20_mph32)
    {
        vehicle_var_1136130 = 16384;
    }

    vehicle_body* veh = vehType2->next_car()->next_car()->next_car()->as_vehicle_body();
    if (veh != nullptr)
    {
        veh->sub_4AAB0B();
    }

    if (var_5D == 1)
    {
        if (!(var_0C & (1 << 1)))
        {
            sub_4B996F();
            sub_4B9987();
            sub_4B99E1();
        }

        sub_4B980A();
        return true;
    }
    else if (var_5D == 5)
    {
        sub_4B9A2A();
        sub_4B980A();
        return true;
    }
    else if (var_5D == 6)
    {
        return sub_4A95F5();
    }

    var_5D = 2;
    uint8_t al = 0;
    uint16_t target_speed = 0;
    sub_4273DF(al, target_speed);

    var_5D = al;
    vehicle_1* vehType1 = vehicle_113611C;
    vehType1->var_44 = target_speed;

    sub_4707C0();

    uint32_t type1speed = speed16_to_32(vehType1->var_44);
    auto type2speed = vehType2->speed;

    if (type2speed == type1speed)
    {
        vehType2->var_5A = 5;

        if (type2speed != 20_mph32)
        {
            vehType2->var_5A = 2;
        }
    }
    else if (type2speed > type1speed)
    {
        vehType2->var_5A = 2;
        uint32_t deceleration_rate = 2_mph32;
        if (type2speed > 130_mph32)
        {
            deceleration_rate = 5_mph32;
            if (type2speed > 400_mph32)
            {
                deceleration_rate = 11_mph32;
                if (type2speed > 600_mph32)
                {
                    deceleration_rate = 25_mph32;
                }
            }
        }

        if (type1speed == 20_mph32)
        {
            vehType2->var_5A = 3;
        }

        type2speed = std::max<int32_t>(0, type2speed - deceleration_rate);
        vehType2->speed = type2speed;
    }
    else
    {
        vehType2->var_5A = 1;
        type2speed += 2_mph32;
        type2speed = std::min<uint32_t>(type2speed, type1speed);
        vehType2->speed = type2speed;
    }

    uint32_t manhattan_distance = 0;
    uint16_t target_z = 0;
    uint8_t target_yaw = 0;
    sub_427122(manhattan_distance, target_z, target_yaw);

    vehicle_var_manhattan_distance_to_station = manhattan_distance;
    vehicle_var_target_z = target_z;

    if (vehicle_var_525BB0 & (1 << 7))
    {
        vehicle_var_113646D = target_yaw;
        target_yaw = sprite_yaw;
        vehType2->var_5A = 1;
        if (target_z < z)
        {
            vehType2->var_5A = 2;
        }
    }

    target_yaw = (target_yaw - sprite_yaw) & 0x3F;

    if (target_yaw != 0)
    {
        if (target_yaw > 32)
        {
            sprite_yaw = (sprite_yaw - 1) & 0x3F;
        }
        else
        {
            sprite_yaw = (sprite_yaw + 1) & 0x3F;
        }
    }

    uint8_t pitch = 0;
    if (vehType2->speed < 50_mph32)
    {
        auto vehObject = vehType2->next_car()->object();
        pitch = 2;
        // Slope sprites for taxiing planes??
        if (!(vehObject->flags & (1 << 8)))
        {
            pitch = 0;
        }
    }

    if (target_z > z)
    {
        if (vehType2->speed <= 350_mph32)
        {
            pitch = 2;
        }
    }

    if (target_z < z)
    {
        if (vehType2->speed <= 180_mph32)
        {
            auto vehObject = vehType2->next_car()->object();

            if (vehObject->flags & (1 << 11))
            {
                pitch = 2;
            }
        }
    }

    if (pitch != sprite_pitch)
    {
        if (pitch < sprite_pitch)
        {
            sprite_pitch--;
        }
        else
        {
            sprite_pitch++;
        }
    }

    if (vehicle_var_525BB0 & (1 << 7))
    {
        vehType2->speed = 8_mph32;
        if (target_z != z)
        {
            return sub_4A94A9(target_z);
        }
    }
    else
    {
        uint32_t param = 480;
        if (var_68 != 0xFF)
        {
            param = 5;
            if (vehType2->speed >= 70_mph32)
            {
                param = 30;
            }
        }

        if (manhattan_distance > param)
        {
            return sub_4A94A9(target_z);
        }
    }

    if (station_id != 0xFFFF && var_68 != 0xFF)
    {
        uint16_t flags = 0;
        sub_426E26(station_id, var_68, flags);

        if (flags & (1 << 8))
        {
            sub_42750E();
        }
        if (flags & (1 << 3))
        {
            sub_4BACAF();
        }

        if (flags & (1 << 0))
        {
            return sub_4A95CB();
        }
    }

    int32_t eax = -1;
    if (station_id != 0xFFFF)
    {
        // This is sign extended just cause sub_427214 needs it to be
        eax = (int8_t)var_68;
    }

    sub_427214(eax);

    if (eax != -2)
    {
        return sub_4A9348(eax, target_z);
    }

    if (vehType2->speed > 30_mph32)
    {
        return sub_4A94A9(target_z);
    }
    else
    {
        vehType2->speed = 0;
        vehType2->var_5A = 0;

        sub_4B980A();
        return true;
    }
}

bool vehicle_0::sub_4A95CB()
{
    if (var_0C & (1 << 1))
    {
        var_5D = 1;
        vehicle_2* vehType2 = vehicle_1136120;
        vehType2->speed = 0;
    }
    else
    {
        sub_4B996F();
        sub_4B9987();
        sub_4B99E1();
    }

    sub_4B980A();
    return true;
}

bool vehicle_0::sub_4A94A9(uint16_t target_z)
{
    auto _yaw = sprite_yaw;
    if (vehicle_var_525BB0 & (1 << 7))
    {
        _yaw = vehicle_var_113646D;
    }

    vehicle_1* vehType1 = vehicle_113611C;
    vehicle_2* vehType2 = vehicle_1136120;

    loc16 loc = {
        x,
        y,
        (int16_t)target_z
    };

    map::map_pos loc2 = {
        vehType1->var_4E,
        vehType1->var_50
    };

    // loc1 and loc2 occupy eax, ecx
    auto factor = vehType2->speed / 8192;
    auto factorX = factorXY503B6A[_yaw * 2] * factor;
    auto factorY = factorXY503B6A[_yaw * 2 + 1] * factor;
    int bigCoordx = (uint16_t)loc.x * 65536 + (uint16_t)loc2.x + factorX;
    int bigCoordy = (uint16_t)loc.y * 65536 + (uint16_t)loc2.y + factorY;

    loc.x = bigCoordx >> 16;
    loc2.x = bigCoordx & 0xFFFF;

    loc.y = bigCoordy >> 16;
    loc2.y = bigCoordy & 0xFFFF;
    
    vehType1->var_4E = loc2.x;
    vehType1->var_50 = loc2.y;
    if (target_z != z)
    {
        if (vehicle_var_manhattan_distance_to_station <= 28)
        {
            int16_t z_shift = 1;
            if (vehType2->speed >= 50_mph32)
            {
                z_shift++;
                if (vehType2->speed >= 100_mph32)
                {
                    z_shift++;
                }
            }

            if (target_z < z)
            {
                loc.z = std::max<int16_t>(target_z, z - z_shift);
            }
            else if (target_z > z)
            {
                loc.z = std::min<int16_t>(target_z, z + z_shift);
            }
        }
        else
        {
            int32_t z_diff = target_z - z;
            int32_t param1 = std::abs(z_diff) * (vehType2->speed / 65536) / 32;
            int32_t param2 = vehicle_var_manhattan_distance_to_station - 18;
            // Crude round up??
            if (z_diff < 0)
            {
                loc.z = z - param1 / param2 - 1;
            }
            else
            {
                loc.z = z + param1 / param2 + 1;
            }
        }
    }
    sub_426CA4(loc, sprite_yaw, sprite_pitch);
    sub_4B980A();
    return true;
}

bool openloco::vehicle_0::sub_4A95F5()
{
    vehicle_2* vehType2 = vehicle_1136120;
    vehType2->speed = 0;
    vehType2->var_5A = 0;
    if (sub_4BA142())
    {
        sub_4B980A();
        return true;
    }

    sub_4707C0();
    var_5D = 2;

    uint8_t al = 0;
    uint16_t bx = 0;
    sub_4273DF(al, bx);

    var_5D = al;

    int32_t eax = -1;
    if (station_id != 0xFFFF)
    {
        // This is sign extended just cause sub_427214 needs it to be
        eax = (int8_t)var_68;
    }

    sub_427214(eax);

    if (eax != -2)
    {
        // Strangely the original would enter this function with an
        // uninitialised target_z. We will pass a valid z.
        return sub_4A9348(eax, z);
    }

    var_5D = 6;
    sub_4B980A();
    return true;
}

bool vehicle_0::sub_4A9348(uint32_t unk_1, uint16_t target_z)
{
    if (station_id != (uint16_t)-1 && var_68 != (uint8_t)-1)
    {
        stationmgr::get(station_id)->var_3BA &= ~(1 << var_68);
    }

    if (unk_1 == (uint32_t)-1)
    {
        sub_4BAC74();
        if (var_4C == 1)
        {
            // 0x4a94a5
            var_68 = 0xFF;
            return sub_4A94A9(target_z);
        }

        uint8_t bl = vehicle_var_987C5C[var_4A + var_46] & 7;

        if (bl != 1)
        {
            var_68 = 0xFF;
            return sub_4A94A9(target_z);
        }

        openloco::station_id_t station_index = (vehicle_var_987C5C[var_4A + var_46 + 1] | ((vehicle_var_987C5C[var_4A + var_46] >> 6) << 8))
            & 0x3FF;

        auto station = stationmgr::get(station_index);

        if (station == nullptr || !(station->var_2A & (1 << 6)))
        {
            var_68 = 0xFF;
            return sub_4A94A9(target_z);
        }

        if (!is_player_company(owner))
        {
            station_id = station_index;
            var_68 = 0xFF;
            return sub_4A94A9(target_z);
        }

        map::map_pos3 loc = {
            station->unk_tile_x,
            station->unk_tile_y,
            station->unk_tile_z
        };

        auto tile = map::tilemgr::get(loc);
        for (auto& el : tile)
        {
            auto elStation = el.as_station();
            if (elStation == nullptr)
                continue;

            if (elStation->base_z() != loc.z / 4)
                continue;

            auto airportObject = objectmgr::get<airport_object>(elStation->object_id());

            uint16_t param = next_car()->next_car()->next_car()->sub_426790();

            if (airportObject->var_10 & param)
            {
                station_id = station_index;
                var_68 = 0xFF;
                return sub_4A94A9(target_z);
            }

            if (owner == companymgr::get_controlling_id())
            {
                messagemgr::post(
                    message_type::unable_to_land_at_airport,
                    owner,
                    var_0A,
                    station_index);
            }

            var_68 = 0xFF;
            return sub_4A94A9(target_z);
        }

        // Todo: fail gracefully on tile not found
        assert(false);

        return true;
        // 0x004A938A
    }
    else
    {
        var_68 = unk_1;
        if (station_id != (uint16_t)-1)
        {
            auto station = stationmgr::get(station_id);
            station->var_3BA |= (1 << var_68);
        }
        return sub_4A94A9(target_z);
    }
}

// 0x004A9649
bool openloco::vehicle_0::update_boat()
{

    vehicle_2* vehType2 = vehicle_1136120;
    if (vehType2->speed >= 5_mph32)
    {
        vehicle_var_1136130 = 16384;
    }
    else
    {
        vehicle_var_1136130 = 8192;
    }

    auto vehBody = vehType2->next_car()->next_car()->next_car()->as_vehicle_body();
    if (vehBody != nullptr)
    {
        vehBody->sub_4AAB0B();
    }

    if (var_5D == 1)
    {
        if (var_0C & (1 << 1))
        {
            sub_4B980A();
            return true;
        }

        if (station_id != uint16_t(-1))
        {
            vehType2->speed = 0;
            sub_4B996F();
            sub_4B9987();
            sub_4BACAF();
            sub_4B99E1();
            sub_4B980A();
            return true;
        }
    }

    if (var_0C & (1 << 1))
    {
        if (!(sub_427C05(1) & (1 << 17)))
        {
            sub_4B980A();
            return true;
        }

        var_5D = 1;
        vehType2->speed = 0;
        vehType2->var_5A = 0;

        sub_4B980A();
        return true;
    }

    if (var_5D == 5)
    {
        sub_4B9A2A();
        sub_4B980A();
        return true;
    }
    else if (var_5D == 6)
    {
        if (sub_4BA142())
        {
            sub_4B980A();
            return true;
        }

        sub_4BAC74();
        sub_4707C0();
        var_5D = 2;
        var_5D = sub_427BF2();
        sub_427C05(2);
        sub_42843E();

        sub_4B980A();
        return true;
    }
    else
    {
        var_5D = 2;
        var_5D = sub_427BF2();
        sub_4707C0();
        if (!(sub_427C05(0) & (1 << 16)))
        {
            sub_4B980A();
            return true;
        }

        if (var_0C & (1 << 1))
        {
            var_5D = 1;
            vehType2->speed = 0;
            vehType2->var_5A = 0;

            sub_4B980A();
            return true;
        }

        vehType2->speed = 0;
        sub_4B996F();
        sub_4B9987();
        sub_4BACAF();
        sub_4B99E1();
        sub_4B980A();
        return true;
    }
}

// 0x004AA1D0
bool openloco::vehicle_body::Update()
{
    if (v_class == vehicle_class::air || v_class == vehicle_class::water)
    {
        animation_update();
        return true;
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
    return true;
}

void openloco::vehicle_body::animation_update()
{
    if (var_38 & (1 << 4))
        return;

    vehicle_0* veh = vehicle_1136118;
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
void openloco::vehicle_body::sub_4AAB0B()
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
        al = (vehType2->speed >> 16) / (vehicle_object->speed / vehicle_object->sprites[object_sprite_type].var_02);
        al = std::min(al, vehicle_object->sprites[object_sprite_type].var_02);
    }
    else if (vehicle_object->sprites[object_sprite_type].var_05 != 1)
    {
        vehicle_bogie* frontBogie = vehicle_front_bogie;
        vehicle_2* vehType2 = vehicle_1136120;
        al = var_46;
        int8_t ah = 0;
        if (vehType2->speed < 35_mph32)
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

void openloco::vehicle_0::sub_4A8882()
{
    vehicle_2* vehType2 = vehicle_1136120;
    sub_4A88A6(vehType2->as_vehicle_2or6());
    vehicle* vehType6 = (vehicle*)vehType2;
    while (vehType6->type != vehicle_thing_type::vehicle_6)
    {
        vehType6 = vehType6->next_car();
    }
    sub_4A88A6(vehType6->as_vehicle_2or6());
}

// Not guaranteed to be type 2 could be type 6
void openloco::vehicle_0::sub_4A88A6(vehicle_26* vehType2or6)
{
    if (tile_x == -1 || var_5D == 8 || var_5D == 9 || (var_38 & (1 << 4)) || vehType2or6->object_id == 0xFFFF)
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

void openloco::vehicle_0::sub_4A8B7C(vehicle_26* vehType2or6)
{
    vehType2or6->sound_id = 0xFF;
}

void openloco::vehicle_0::sub_4A88F7(vehicle_26* vehType2or6, vehicle_object_sound_1* snd)
{
    vehicle_2* vehType2_2 = vehicle_1136120;
    if (vehType2_2->speed < snd->var_01)
    {
        sub_4A8B7C(vehType2or6);
        return;
    }

    uint32_t unk = vehType2_2->speed - snd->var_01;
    vehType2or6->var_46 = (unk >> snd->var_05) + snd->var_06;

    unk >>= snd->var_08;
    unk += snd->var_09;

    if (unk > snd->var_0A)
    {
        unk = snd->var_0A;
    }

    vehType2or6->var_45 = unk;
    vehType2or6->sound_id = snd->sound_object_id;
}

void openloco::vehicle_0::sub_4A8937(vehicle_26* vehType2or6, vehicle_object_sound_2* snd)
{
    if ((vehicle_2*)vehType2or6 == vehicle_1136120)
    {
        if (v_type != vehicle_type::ship && v_type != vehicle_type::plane)
        {
            // Can be a type 6 or bogie
            if (vehType2or6->next_car()->var_5F & (1 << 2))
            {
                sub_4A8B7C(vehType2or6);
                return;
            }
        }
    }

    vehicle_2* vehType2_2 = vehicle_1136120;
    uint16_t _var_46 = 0;
    uint8_t _var_45 = 0;
    if (vehType2_2->var_5A == 2)
    {
        if (vehType2_2->speed < 12_mph32)
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
        if (vehType2or6->type == vehicle_thing_type::vehicle_2 || vehType2or6->next_car()->var_5E == 0)
        {
            _var_46 = snd->var_07 + (vehType2_2->speed >> snd->var_10);
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

    if (vehType2or6->sound_id == 0xFF)
    {
        // Half
        vehType2or6->var_45 = snd->var_03 >> 1;
        // Quarter
        vehType2or6->var_46 = snd->var_01 >> 2;
        vehType2or6->sound_id = snd->sound_object_id;
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

    vehType2or6->sound_id = snd->sound_object_id;
}

void openloco::vehicle_0::sub_4A8A39(vehicle_26* vehType2or6, vehicle_object_sound_3* snd)
{
    if ((vehicle_2*)vehType2or6 == vehicle_1136120)
    {
        if (v_type != vehicle_type::ship && v_type != vehicle_type::plane)
        {
            // Can be a type 6 or bogie
            if (vehType2or6->next_car()->var_5F & (1 << 2))
            {
                sub_4A8B7C(vehType2or6);
                return;
            }
        }
    }

    vehicle_2* vehType2_2 = vehicle_1136120;
    uint16_t _var_46 = 0;
    uint8_t _var_45 = 0;
    bool var5aEqual1Code = false;

    if (vehType2_2->var_5A == 2 || vehType2_2->var_5A == 3)
    {
        if (vehType2_2->speed < 12_mph32)
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
        if (vehType2or6->type == vehicle_thing_type::vehicle_2 || vehType2or6->next_car()->var_5E == 0)
        {
            auto _var_56 = std::min(vehType2_2->speed, (uint32_t)7_mph32) >> 16;

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

    if (vehType2or6->sound_id == 0xFF)
    {
        // Half
        vehType2or6->var_45 = snd->var_03 >> 1;
        // Quarter
        vehType2or6->var_46 = snd->var_01 >> 2;
        vehType2or6->sound_id = snd->sound_object_id;
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

    vehType2or6->sound_id = snd->sound_object_id;
}

// 0x004AC255
void openloco::vehicle_body::sub_4AC255(vehicle_bogie* back_bogie, vehicle_bogie* front_bogie)
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
uint16_t openloco::vehicle_body::sub_4BE368(uint32_t distance)
{
    uint8_t i = 10;
    for (; distance > 4096; --i, distance >>= 2)
        ;

    return vehicle_arr_500B50[distance >> 1] >> i;
}

// 0x004BF4DA
uint8_t openloco::vehicle_body::vehicle_body_update_sprite_pitch_steep_slopes(uint16_t xy_offset, int16_t z_offset)
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
uint8_t openloco::vehicle_body::vehicle_body_update_sprite_pitch(uint16_t xy_offset, int16_t z_offset)
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
uint8_t openloco::vehicle_body::vehicle_update_sprite_yaw_0(int16_t x_offset, int16_t y_offset)
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

// 0x00503E6E
constexpr uint8_t indexToYaw_12[] = {
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

// 0x004BF5B3
uint8_t openloco::vehicle_body::vehicle_update_sprite_yaw_1(int16_t x_offset, int16_t y_offset)
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

    return indexToYaw_12[i];
}

// 0x004BF5FB
uint8_t openloco::vehicle_body::vehicle_update_sprite_yaw_2(int16_t x_offset, int16_t y_offset)
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
uint8_t openloco::vehicle_body::vehicle_update_sprite_yaw_3(int16_t x_offset, int16_t y_offset)
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
uint8_t openloco::vehicle_body::vehicle_update_sprite_yaw_4(int16_t x_offset, int16_t y_offset)
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
void openloco::vehicle_body::secondary_animation_update()
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
void openloco::vehicle_body::steam_puffs_animation_update(uint8_t num, int8_t var_05)
{
    auto vehicleObject = object();
    vehicle_bogie* frontBogie = vehicle_front_bogie;
    vehicle_bogie* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* vehType2 = vehicle_1136120;
    bool soundCode = false;
    if (vehType2->var_5A == 1 || vehType2->var_5A == 4)
    {
        soundCode = true;
    }
    bool tickCalc = true;
    if (vehType2->var_5A != 0 && vehType2->speed >= 1_mph32)
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

    bool station_found = false;

    // Looking for a bridge? or somthing ontop
    if (steam_obj->var_08 & (1 << 2))
    {
        auto tile = map::tilemgr::get(frontBogie->tile_x, frontBogie->tile_y);

        for (auto& el : tile)
        {
            auto elUnk1 = el.as_unk1();
            if (elUnk1 == nullptr)
                continue;

            if (elUnk1->base_z() != frontBogie->tile_base_z)
                continue;

            if (elUnk1->unk_direction() != (frontBogie->var_2C & 0x3))
                continue;

            if (elUnk1->unk_4() != ((frontBogie->var_2C >> 3) & 0x3F))
                continue;

            if (!elUnk1->has_station_element())
                continue;

            auto elStation = el.next();
            if (elStation == nullptr)
                continue;
            
            if (elStation->flags() & (map::element_flags::flag_5 | map::element_flags::flag_4))
                continue;
            station_found = true;
            break;
        }
    }

    if (station_found)
    {
        auto soundId = static_cast<audio::sound_id>(steam_obj->var_1F[var_55 + (steam_obj->sound_effect >> 1)]);

        if (vehType2->speed > 15_mph32)
            return;

        int32_t volume = 0 - (vehType2->speed >> 9);

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

        if (vehType2->speed > 15_mph32)
            return;

        int32_t volume = 0 - (vehType2->speed >> 9);

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
void openloco::vehicle_body::diesel_exhaust1_animation_update(uint8_t num, int8_t var_05)
{
    vehicle_bogie* frontBogie = vehicle_front_bogie;
    vehicle_bogie* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_0* veh = vehicle_1136118;
    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (veh->v_type == vehicle_type::ship)
    {
        if (vehType2->speed == 0)
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
void openloco::vehicle_body::diesel_exhaust2_animation_update(uint8_t num, int8_t var_05)
{
    vehicle_bogie* frontBogie = vehicle_front_bogie;
    vehicle_bogie* backBogie = vehicle_back_bogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (vehType2->var_5A != 1)
        return;

    if (vehType2->speed > 14_mph32)
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
void openloco::vehicle_body::electric_spark1_animation_update(uint8_t num, int8_t var_05)
{
    vehicle_bogie* frontBogie = vehicle_front_bogie;
    vehicle_bogie* backBogie = vehicle_back_bogie;
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
void openloco::vehicle_body::electric_spark2_animation_update(uint8_t num, int8_t var_05)
{
    vehicle_bogie* frontBogie = vehicle_front_bogie;
    vehicle_bogie* backBogie = vehicle_back_bogie;
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
void openloco::vehicle_body::ship_wake_animation_update(uint8_t num, int8_t var_05)
{
    vehicle_2* vehType2 = vehicle_1136120;
    auto vehicleObject = object();

    if (vehType2->var_5A == 0)
        return;

    if (vehType2->speed < 6_mph32)
        return;

    auto frequency = 32;
    if (vehType2->speed >= 9_mph32)
    {
        frequency = 16;
        if (vehType2->speed >= 13_mph32)
        {
            frequency = 8;
            if (vehType2->speed >= 25_mph32)
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

void vehicle_0::sub_4AF06E()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004AF06E, regs);
}

void vehicle_0::sub_4B9A2A()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004B9A2A, regs);
}

void vehicle_0::sub_4B980A()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004B980A, regs);
}

void vehicle_0::sub_4AA625()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004AA625, regs);
}

void vehicle_0::sub_4AD93A()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004AD93A, regs);
}

int16_t vehicle_0::sub_4BABAD()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004AD93A, regs);
    return regs.ax;
}

void vehicle_0::sub_4B996F()
{
    auto station = stationmgr::get(station_id);
    station->var_3B2 |= (1 << (uint8_t)v_type);
}

void vehicle_0::sub_4B9987()
{
    uint8_t al = vehicle_var_987C5C[var_4A + var_46] & 7;

    if (al != 1 && al != 2)
    {
        return;
    }

    station_id_t station_id2 = vehicle_var_987C5C[var_4A + var_46 + 1] | ((vehicle_var_987C5C[var_4A + var_46] >> 6) << 8);

    if (station_id2 != station_id)
    {
        return;
    }

    auto unk_1 = var_4A + vehicle_var_4FE070[al];

    if (vehicle_var_987C5C[unk_1 + var_46] == 0)
    {
        unk_1 = 0;
    }

    var_4A = unk_1;
    ui::windowmgr::sub_4B93A5(var_0A);
}

void vehicle_0::sub_4BACAF()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004BACAF, regs);
}

void vehicle_0::sub_4B99E1()
{
    var_5F &= ~(1 << 0);
    var_5D = 5;
    var_56 = 10;
    var_58 = 0;

    auto veh = next_car()->next_car()->next_car();
    while (veh->type != vehicle_thing_type::vehicle_6)
    {
        veh->var_5F |= (1 << 0);
        veh = veh->next_car();
    }
}

void vehicle_0::sub_4707C0()
{
    if (var_4C == 1)
    {
        return;
    }

    uint8_t bl = vehicle_var_987C5C[var_4A + var_46] & 7;

    if (vehicle_var_4FE088[bl] & (1 << 0))
    {
        return;
    }

    auto _var_4A = var_4A;

    do
    {
        _var_4A += vehicle_var_4FE070[bl];
        if (bl == 0)
        {
            _var_4A = 0;
        }

        if (_var_4A == var_4A)
        {
            return;
        }

        bl = vehicle_var_987C5C[_var_4A + var_46] & 7;

    } while (!(bl & (1 << 0)));

    if (_var_4A != var_4A)
    {
        var_4A = _var_4A;
        ui::windowmgr::sub_4B93A5(var_0A);
    }
}

void vehicle_0::sub_4ACEE7(uint32_t unk_1, uint32_t var_113612C, uint8_t& unk_2, uint8_t& unk_3, uint16_t& unk_4)
{
    registers regs;
    regs.esi = (int32_t)this;
    regs.eax = unk_1;
    regs.ebx = var_113612C;
    call(0x004ACEE7, regs);
    unk_2 = regs.al;
    unk_3 = regs.ah;
    unk_4 = regs.bp;
}

bool vehicle_0::sub_4AC1C2()
{
    //vehicle_var_113601A = 0;
    //auto loc_x = tile_x;
    //auto loc_y = tile_y;
    //auto loc_z = tile_base_z * 4;
    //auto ebp = var_2C;
    //auto bl = owner;
    //auto bh = road_object_id;
    //auto edi = &vehicle_var_11360A0;
    registers regs;
    regs.esi = (int32_t)this;
    return call(0x004AC1C2, regs) & (1 << 8);
}

bool vehicle_0::sub_4AC0A3()
{
    registers regs;
    regs.esi = (int32_t)this;
    return call(0x004AC0A3, regs) & (1 << 8);
}

void vehicle_0::sub_4AD778()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004AD778, regs);
}

// 0x004AA36A
uint8_t vehicle_0::sub_4AA36A()
{
    vehicle* veh = next_car()->next_car();
    if (veh->var_36 != var_36 || veh->var_2E != var_2E)
    {
        veh->var_46 = 0;
        return 0;
    }

    auto param1 = 160;
    auto param2 = 960;

    if (road_object_id == 0xFF || objectmgr::get<road_object>(road_object_id)->var_12 & (1 << 6))
    {
        if (veh->var_2C & (1 << 7))
        {
            param1 = 128;
            param2 = 544;
        }
    }
    else
    {
        param2 = 2880;
        if (veh->var_2C & (1 << 7))
        {
            param1 = 64;
            param2 = 128;
        }
    }

    veh->var_46++;
    if (veh->var_46 == param1)
    {
        return 1;
    }

    if (veh->var_46 == param2)
    {
        var_5C = 40;
        return 2;
    }

    return 0;
}

bool vehicle_0::sub_4BADE4()
{
    vehicle* veh = next_car()->next_car();
    map::map_pos3 loc = {
        veh->tile_x,
        veh->tile_y,
        veh->tile_base_z
    };

    uint16_t bp = veh->var_2C;

    if (veh->v_class == vehicle_class::road)
    {
        auto tile = map::tilemgr::get(loc);
        for (auto& el : tile)
        {
            auto elUnk7 = el.as_unk7();
            if (elUnk7 == nullptr)
                continue;

            auto heightDiff = std::abs(elUnk7->base_z() - loc.z);
            if (heightDiff > 4)
                continue;

            if (elUnk7->flags() & ((1 << 4) | (1 << 5)))
                continue;

            if (elUnk7->unk_4_F() != ((bp >> 3) & 0xF))
                continue;

            return true;
        }
        return false;
    }
    else
    {
        auto tile = map::tilemgr::get(loc);
        for (auto& el : tile)
        {
            auto elUnk1 = el.as_unk1();
            if (elUnk1 == nullptr)
                continue;

            auto heightDiff = std::abs(elUnk1->base_z() - loc.z);
            if (heightDiff > 4)
                continue;

            if (elUnk1->flags() & ((1 << 4) | (1 << 5)))
                continue;

            if (elUnk1->unk_direction() != (bp & 0x3))
                continue;

            if (elUnk1->unk_4() != ((bp >> 3) & 0x3F))
                continue;

            return true;
        }
        return false;
    }
}

void vehicle_0::sub_4ADB47(uint32_t unk_1)
{
    registers regs;
    regs.esi = (int32_t)this;
    regs.eax = unk_1;
    call(0x004ADB47, regs);
}

void vehicle_2::sub_4AA464()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004ADB47, regs);
}

// Wait for load. False on loaded
bool vehicle_0::sub_4BA142()
{
    registers regs;
    regs.esi = (int32_t)this;
    return call(0x004BA142, regs) & (1 << 8);
}

void vehicle_0::sub_4BAC74()
{
    var_73 = scenario_ticks();

    vehicle* veh = next_car()->next_car();
    var_6F = veh->x;
    var_71 = veh->y;

    var_5F |= (1 << 3);
}

bool vehicle_0::sub_4ACCDC()
{
    if (v_class == vehicle_class::road)
    {
        // 0x004ACDE0
    }
    else
    {
        auto veh = next_car()->as_vehicle_1();
        vehicle_var_113601A[0] = var_53;
        vehicle_var_113601A[1] = veh->var_49;

        loc16 loc = {
            tile_x,
            tile_y,
            tile_base_z * 4
        };

        // 0x011360A0 for array data
        sub_4A2604(loc, owner, road_object_id, var_2C, (uint16_t*)&vehicle_var_array_11360A0->data);

        if (vehicle_var_array_11360A0->size == 0)
        {
            return false;
        }
        // 0x004ACD1F
    }
    registers regs;
    regs.esi = (int32_t)this;
    return call(0x004ACCDC, regs) & (1 << 8);
}

void vehicle_0::sub_4273DF(uint8_t& unk_1, uint16_t& target_speed16)
{
    if (station_id == 0xFFFF || var_68 == 0xFF)
    {
        auto veh = next_car()->next_car()->as_vehicle_2();

        if (veh == nullptr)
        {
            assert(false);
            return;
        }
        unk_1 = 2;
        target_speed16 = veh->speed_54;

        if (veh->var_73 & flags_73::broken_down)
        {
            target_speed16 = veh->speed_5C;
        }

        return;
    }

    auto station = stationmgr::get(station_id);

    map::map_pos3 loc = {
        station->unk_tile_x,
        station->unk_tile_y,
        station->unk_tile_z
    };

    auto tile = map::tilemgr::get(loc);
    for (auto& el : tile)
    {
        auto elStation = el.as_station();
        if (elStation == nullptr)
            continue;

        if (elStation->base_z() != loc.z / 4)
            continue;

        auto airportObject = objectmgr::get<airport_object>(elStation->object_id());

        uint8_t al = airportObject->var_B2[var_68].var_03;
        uint8_t cl = airportObject->var_B2[var_68].var_00;

        auto veh = next_car()->next_car()->as_vehicle_2();
        if (veh == nullptr)
        {
            assert(false);
            return;
        }

        if (al != 0)
        {
            if (cl == 1 || al != 2)
            {
                if (al == 1)
                {
                    unk_1 = 10;
                    target_speed16 = veh->speed_5C;
                }
                else if (al == 3)
                {
                    unk_1 = 10;
                    target_speed16 = 0;
                }
                else if (al == 4)
                {
                    unk_1 = 11;
                    target_speed16 = 20;
                }
                else
                {
                    unk_1 = 4;
                    target_speed16 = veh->speed_5C;
                }
                return;
            }
        }

        if (cl == 2)
        {
            unk_1 = 13;
            target_speed16 = veh->speed_54;
            if (veh->var_73 & flags_73::broken_down)
            {
                target_speed16 = veh->speed_5C;
            }
        }
        else if (cl == 3)
        {
            unk_1 = 13;
            target_speed16 = 0;
        }
        else
        {
            unk_1 = 12;
            target_speed16 = 20;
        }
        return;
    }

    // Tile not found. Todo: fail gracefully
    assert(false);
}

/**
 * Seems to work out where to land or something like that.
 *  manhattan_distance = regs.ebp
 *  target_z = regs.dx
 *  target_yaw = regs.bl
 *  airportFlags = vehicle_var_525BB0
 */
void vehicle_0::sub_427122(uint32_t& manhattan_distance, uint16_t& target_z, uint8_t& target_yaw)
{
    vehicle_var_525BB0 = 0;
    station_id_t station_id2 = station_id::null;
    map::map_pos3 loc;
    bool loc_found = false;
    if (station_id == station_id::null)
    {
        uint8_t bl = vehicle_var_987C5C[var_4A + var_46] & 7;
        if (bl == 3 || !(vehicle_var_4FE088[bl] & (1 << 3)))
        {
            station_id2 = station_id;
        }
        else
        {
            station_id2 = vehicle_var_987C5C[var_4A + var_46 + 1] | ((vehicle_var_987C5C[var_4A + var_46] >> 6) << 8);
        }
    }
    else
    {
        if (var_68 == (uint8_t)0xFF)
        {
            station_id2 = station_id;
        }
        else
        {
            auto station_ = stationmgr::get(station_id);
            if (!(station_->var_2A & (1 << 6)))
            {
                station_id2 = station_id;
            }
            else
            {
                uint16_t flags = 0;
                sub_426E26(station_id, var_68, flags, loc);
                vehicle_var_525BB0 = flags;
                loc_found = true;
            }
        }
    }

    if (loc_found == false)
    {
        if (station_id2 == (station_id_t)0xFFFF)
        {
            loc.x = 0x17FF;
            loc.y = 0x17FF;
            loc.z = 960;
        }
        else
        {
            auto station_ = stationmgr::get(station_id2);
            loc.x = station_->tile_x;
            loc.y = station_->tile_y;
            if (station_->var_2A & (1 << 6))
            {
                loc.x = station_->unk_tile_x;
                loc.y = station_->unk_tile_y;
            }
            loc.z = 960;
        }
    }

    loc.x -= x;
    loc.y -= y;

    target_yaw = plane_get_yaw_from_loc(loc);

    // manhattan distance to station
    manhattan_distance = std::abs(loc.x) + std::abs(loc.y);

    target_z = loc.z;
}

/**
 * 0x004BF568
 * Works out what Yaw a plane should have to change direction to loc
 */
uint8_t vehicle_0::plane_get_yaw_from_loc(const map::map_pos& loc)
{
    uint8_t index = 0;
    if (loc.x < 0)
    {
        index = 3;
    }

    if (loc.y < 0)
    {
        index += 6;
    }

    uint32_t param2;
    if (loc.y == 0)
    {
        param2 = std::numeric_limits<uint32_t>::max();
    }
    else
    {
        param2 = std::abs(loc.x) * 65536 / std::abs(loc.y);
    }

    if (param2 >= 3434)
    {
        index++;
        if (param2 >= 1250501)
        {
            index++;
        }
    }

    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    call(0x004BF56B, regs);
    // Todo: Remove call after 18/08/18
    assert(regs.eax == indexToYaw_12[index]);
    return indexToYaw_12[index];
}

void vehicle_0::sub_426E26(uint16_t _station_Id, uint8_t unk_var_68, uint16_t& airportFlags)
{
    map::map_pos3 loc = {
        0,
        0,
        0
    };
    sub_426E26(_station_Id, unk_var_68, airportFlags, loc);
}

void vehicle_0::sub_426E26(uint16_t _station_Id, uint8_t unk_var_68, uint16_t& airportFlags, map::map_pos3& loc)
{
    auto station = stationmgr::get(_station_Id);

    map::map_pos3 staion_loc = {
        station->unk_tile_x,
        station->unk_tile_y,
        station->unk_tile_z
    };

    auto tile = map::tilemgr::get(staion_loc);

    for (auto& el : tile)
    {
        auto elStation = el.as_station();
        if (elStation == nullptr)
            continue;

        if (elStation->base_z() != staion_loc.z / 4)
            continue;

        auto airportObject = objectmgr::get<airport_object>(elStation->object_id());

        uint32_t ebx = airportObject->var_B2[unk_var_68].var_02;

        loc = {
            (int16_t)(airportObject->var_AE[ebx].x - 16),
            (int16_t)(airportObject->var_AE[ebx].y - 16),
            (int16_t)(airportObject->var_AE[ebx].z + staion_loc.z)
        };

        airportFlags = airportObject->var_AE[ebx].flags;

        switch (elStation->rotation())
        {
            case 0:
                break;
            case 1:
                std::swap(loc.x, loc.y);
                loc.y = -loc.y;
                break;
            case 2:
                loc.x = -loc.x;
                loc.y = -loc.y;
                break;
            case 3:
                std::swap(loc.x, loc.y);
                loc.x = -loc.x;
                break;
        }

        loc.x += 16 + staion_loc.x;
        loc.y += 16 + staion_loc.y;

        if (!(airportFlags & (1 << 3)))
        {
            loc.z = staion_loc.z + 255;
            if (!(airportFlags & (1 << 4)))
            {
                loc.z = 960;
            }
        }

        // ax, cx, dx = loc
        // airportFlags = ebx = airportObject->var_AE[ebx].flags
        // ebp = &airportObject->var_B2[unk_var_68] ??

        return;
    }

    // Tile not found. Todo: fail gracefully
    assert(false);
}

void vehicle_0::sub_42750E()
{
    vehicle_2* vehType2 = vehicle_1136120;
    auto veh = vehType2->next_car();

    auto vehObject = veh->object();

    if (vehObject->num_sounds == 0)
    {
        return;
    }

    auto chosenSound = gprng().rand_next(vehObject->num_sounds & 0x7F);
    auto soundId = (audio::sound_id)vehObject->var_15B[chosenSound];

    loc16 loc = {
        vehType2->x,
        vehType2->y,
        static_cast<int16_t>(vehType2->z + 22)
    };

    audio::play_sound(soundId, loc, 0, 22050, true);
}

void vehicle_0::sub_427214(int32_t& _var_68)
{
    auto station = stationmgr::get(station_id);

    map::map_pos3 loc = {
        station->unk_tile_x,
        station->unk_tile_y,
        station->unk_tile_z
    };

    auto tile = map::tilemgr::get(loc);

    for (auto& el : tile)
    {
        auto elStation = el.as_station();
        if (elStation == nullptr)
            continue;

        if (elStation->base_z() != loc.z / 4)
            continue;

        auto airportObject = objectmgr::get<airport_object>(elStation->object_id());

        if (_var_68 == -1)
        {
            for (uint8_t b2_index = 0; b2_index < airportObject->var_AD; b2_index++)
            {
                auto _b2struct = &airportObject->var_B2[b2_index];
                if (!(airportObject->var_AE[_b2struct->var_01].flags & (1 << 2)))
                {
                    continue;
                }

                if (station->var_3BA & _b2struct->var_04)
                {
                    continue;
                }

                uint32_t unk = _b2struct->var_08;

                if (unk == 0)
                {
                    _var_68 = b2_index;
                    return;
                }

                unk &= station->var_3BA;
                if (unk == _b2struct->var_08)
                {
                    continue;
                }

                _var_68 = b2_index;
                return;
            }
            _var_68 = -2;
            return;
        }
        else
        {
            uint8_t al = airportObject->var_B2[_var_68].var_02;
            if (var_5D == 13 && airportObject->var_AE[al].flags & (1 << 1))
            {
                _var_68 = -1;
                return;
            }
            // 0x4272A5
            if (next_car()->next_car()->next_car()->object()->flags & (1 << 13))
            {
                for (uint8_t b2_index = 0; b2_index < airportObject->var_AD; b2_index++)
                {
                    auto _b2struct = &airportObject->var_B2[b2_index];

                    if (_b2struct->var_01 != al)
                    {
                        continue;
                    }

                    if (airportObject->var_AE[_b2struct->var_02].flags & (1 << 6))
                    {
                        continue;
                    }

                    if (station->var_3BA & _b2struct->var_04)
                    {
                        continue;
                    }

                    uint32_t unk = _b2struct->var_08;

                    if (unk == 0)
                    {
                        _var_68 = b2_index;
                        return;
                    }

                    unk &= station->var_3BA;
                    if (unk == _b2struct->var_08)
                    {
                        continue;
                    }

                    _var_68 = b2_index;
                    return;
                }

                _var_68 = -2;
                return;
            }
            else
            {
                for (uint8_t b2_index = 0; b2_index < airportObject->var_AD; b2_index++)
                {
                    auto _b2struct = &airportObject->var_B2[b2_index];
                    if (_b2struct->var_01 != al)
                    {
                        continue;
                    }

                    if (airportObject->var_AE[_b2struct->var_02].flags & (1 << 5))
                    {
                        continue;
                    }

                    if (station->var_3BA & _b2struct->var_04)
                    {
                        continue;
                    }

                    uint32_t unk = _b2struct->var_08;

                    if (unk == 0)
                    {
                        _var_68 = b2_index;
                        return;
                    }

                    unk &= station->var_3BA;
                    if (unk == _b2struct->var_08)
                    {
                        continue;
                    }

                    _var_68 = b2_index;
                    return;
                }
                _var_68 = -2;
                return;
            }
        }
    }

    // Tile not found. Todo: fail gracefully
    assert(false);
    return;
}

void vehicle_0::sub_426CA4(loc16 loc, uint8_t yaw, uint8_t pitch)
{
    registers regs;
    regs.esi = (int32_t)this;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.bl = yaw;
    regs.bh = pitch;
    call(0x00426CA4, regs);
}

uint16_t vehicle::sub_426790()
{
    auto obj = object();
    if (obj->flags & (1 << 13))
    {
        return 16;
    }

    if (obj->weight < 60)
    {
        return 12;
    }
    else
    {
        return 8;
    }
}

uint8_t vehicle_0::sub_427BF2()
{
    if (station_id == (uint16_t)-1)
    {
        return 2;
    }
    else
    {
        return 4;
    }
}

uint32_t vehicle_0::sub_427C05(uint32_t unk_1)
{
    registers regs;
    regs.esi = (int32_t)this;
    regs.eax = unk_1;
    call(0x00427C05, regs);
    return regs.eax;
}

void vehicle_0::sub_42843E()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x0042843E, regs);
}

/**
 *
 * loc            = ax, bx, dx (large value x4) 
 * owner          = bl
 * road_object_id = bh
 * _var_2C        = ebp
 * unk_1          = edi
 */
void openloco::sub_4A2604(loc16 &loc, uint8_t owner, uint8_t road_object_id, uint16_t _var_2C, uint16_t * unk_1)
{
    // Get next tile in current direction
    loc.x += vehicle_var_4F7B5C[_var_2C].x;
    loc.y += vehicle_var_4F7B5C[_var_2C].y;
    loc.z += vehicle_var_4F7B5C[_var_2C].z;

    vehicle_var_1135FAE = (station_id_t)-1;
    vehicle_var_113607D = 0;

    map::map_pos3 map_loc = {
        loc.x,
        loc.y,
        loc.z / 4
    };
    sub_4A2601(map_loc, owner, road_object_id, _var_2C, unk_1, vehicle_var_4F7B5C[_var_2C].unk_1);
}

/**
 *
 * loc            = ax, bx, dl (smaller value /4)
 * owner          = bl
 * road_object_id = bh
 * _var_2C        = ebp
 * unk_1          = edi
 * unk_2          = dh
 */
void openloco::sub_4A2601(map::map_pos3 loc, uint8_t owner, uint8_t road_object_id, uint16_t _var_2C, uint16_t * unk_1, uint8_t unk_2)
{
    // Get the array. Remove this hack when hook removed
    array_113609C *unk_arr = (array_113609C*)(unk_1 - 2);
    // Unsure if this is still used. Was used to work out size of array
    vehicle_var_1135EE6 = unk_arr->data;

    auto tile = map::tilemgr::get(loc);
    auto i = 0;
    for (auto& el : tile)
    {
        auto elUnk1 = el.as_unk1();

        if (elUnk1 == nullptr)
        {
            continue;
        }

        if (elUnk1->owner() != owner)
        {
            continue;
        }

        if (elUnk1->road_object_id() != road_object_id)
        {
            continue;
        }

        if ((elUnk1->unk_7u() & vehicle_var_113601A[0]) != vehicle_var_113601A[0])
        {
            continue;
        }

        if (elUnk1->flags() & (map::element_flags::flag_4 | map::element_flags::flag_5))
        {
            continue;
        }

        auto elNext = el.next();
        map::station_element* elStation = nullptr;
        map::unk3_element* elSignal = nullptr;
        if (elNext != nullptr)
        {
            elStation = elNext->as_station();
            elSignal = elNext->as_unk3();
        }
        uint16_t cl = (elUnk1->unk_4() << 3) | elUnk1->unk_direction();

        if (!(elUnk1->unk_5l()) &&
            unk_2 == vehicle_var_4F7B5C[cl].unk_0)
        {
            auto ebp = vehicle_var_4F73D8[elUnk1->unk_4()];
            // Shift arithmatic
            auto height = -(ebp->var_05 >> 2) + elUnk1->base_z();

            if (height == loc.z)
            {
                if (elUnk1->has_4_80())
                {
                    cl |= ((elUnk1->unk_6() & 0xE0) << 4) | (1 << 12);
                }

                if (vehicle_var_113601A[1] & elUnk1->unk_7u())
                {
                    cl |= (1 << 13);
                }

                if (elUnk1->has_station_element() &&
                    elStation != nullptr &&
                    !(elStation->flags() & (map::element_flags::flag_4 | map::element_flags::flag_5)))
                {
                    vehicle_var_1135FAE = elStation->station_id();
                }

                if (elUnk1->unk_6() & (1 << 4))
                {
                    vehicle_var_113607D = (1 << 0);
                }

                if (elUnk1->has_signal() &&
                    elSignal != nullptr &&
                    !(elSignal->flags() & (map::element_flags::flag_4 | map::element_flags::flag_5)))
                {
                    cl |= (1 << 15);
                }

                unk_arr->data[i++] = cl;
                cl &= 0x1FB;
            }
        }

        if (!(elUnk1->flags() & (map::element_flags::flag_6)))
        {
            continue;
        }

        cl |= (1 << 2);
        if (unk_2 != vehicle_var_4F7B5C[cl].unk_0)
        {
            continue;
        }

        // Pretty sure these are identical
        assert(elUnk1->unk_4() == (cl >> 3));
        auto ebp = vehicle_var_4F73D8[elUnk1->unk_4()];
        ebp += elUnk1->unk_5l();
        // Shift arithmatic
        auto height = -((ebp->var_05 + vehicle_var_4F7B5C[cl].z) >> 2) + elUnk1->base_z();

        if (height != loc.z)
        {
            continue;
        }

        if (elUnk1->has_4_80())
        {
            cl |= ((elUnk1->unk_6() & 0xE0) << 4) | (1 << 12);
        }

        if (vehicle_var_113601A[1] & elUnk1->unk_7u())
        {
            cl |= (1 << 13);
        }

        if (elUnk1->has_station_element() &&
            elStation != nullptr &&
            !(elStation->flags() & (map::element_flags::flag_4 | map::element_flags::flag_5)))
        {
            vehicle_var_1135FAE = elStation->station_id();
        }

        if (elUnk1->unk_6() & (1 << 4))
        {
            vehicle_var_113607D = (1 << 0);
        }

        if (elUnk1->has_signal() &&
            elSignal != nullptr &&
            !(elSignal->flags() & (map::element_flags::flag_4 | map::element_flags::flag_5)))
        {
            cl |= (1 << 15);
        }

        unk_arr->data[i++] = cl;
    }

    unk_arr->data[i] = (uint16_t)-1;
    unk_arr->size = i;
}