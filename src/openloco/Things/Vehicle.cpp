#include "Vehicle.h"
#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../Utility/Numeric.hpp"
#include "../ViewportManager.h"
#include "Misc.h"
#include "ThingManager.h"
#include <algorithm>
#include <cassert>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::objectmgr;

loco_global<vehicle_head*, 0x01136118> vehicleUpdate_head;
loco_global<vehicle_bogie*, 0x01136124> vehicleUpdate_frontBogie;
loco_global<vehicle_bogie*, 0x01136128> vehicleUpdate_backBogie;
loco_global<int32_t, 0x01136130> vehicle_var_1136130;
loco_global<vehicle_2*, 0x01136120> vehicleUpdate_2;
loco_global<uint8_t, 0x01136237> vehicle_var_1136237;   // var_28 related?
loco_global<uint8_t, 0x01136238> vehicle_var_1136238;   // var_28 related?
loco_global<int8_t[88], 0x004F865C> vehicle_arr_4F865C; // var_2C related?
loco_global<uint16_t[2047], 0x00500B50> vehicle_arr_500B50;
loco_global<int16_t[128], 0x00503B6A> factorXY503B6A;
loco_global<uint8_t[44], 0x004F8A7C> vehicle_arr_4F8A7C; // bools
loco_global<uint8_t, 0x00525FAE> vehicle_var_525FAE;     // boolean
static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;

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

vehicle* vehicle::nextVehicle()
{
    return thingmgr::get<vehicle>(next_thing_id);
}

vehicle* vehicle::nextVehicleComponent()
{
    return thingmgr::get<vehicle>(next_car_id);
}

vehicle_object* vehicle::object() const
{
    return objectmgr::get<vehicle_object>(object_id);
}

vehicle_object* vehicle_body::object() const
{
    return objectmgr::get<vehicle_object>(object_id);
}

void vehicle_head::updateVehicle()
{
    // TODO: Refactor to use the Vehicle super class
    auto v = reinterpret_cast<vehicle*>(this);
    while (v != nullptr)
    {
        if (v->updateComponent())
        {
            break;
        }
        v = v->nextVehicleComponent();
    }
}

uint16_t vehicle_head::update()
{
    registers regs;
    regs.esi = (int32_t)this;
    return call(0x004A8B81, regs);
}

bool vehicle::updateComponent()
{
    int32_t result = 0;
    registers regs;
    regs.esi = (int32_t)this;
    switch (type)
    {
        case VehicleThingType::head:
            result = asVehicleHead()->update();
            break;
        case VehicleThingType::vehicle_1:
            result = call(0x004A9788, regs);
            break;
        case VehicleThingType::vehicle_2:
            result = call(0x004A9B0B, regs);
            break;
        case VehicleThingType::bogie:
            result = call(0x004AA008, regs);
            break;
        case VehicleThingType::body_start:
        case VehicleThingType::body_continued:
            result = asVehicleBody()->update();
            break;
        case VehicleThingType::tail:
            result = call(0x004AA24A, regs);
            break;
        default:
            break;
    }
    return (result & (1 << 8)) != 0;
}

// 0x004BA8D4
void vehicle_head::sub_4BA8D4()
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
    things::vehicle::Vehicle train(this);
    for (auto car : train.cars)
    {
        if (car.front->var_5F & flags_5f::broken_down)
        {
            if ((scenarioTicks() & 3) == 0)
            {
                auto v2 = car.body; // body
                smoke::create(loc16(v2->x, v2->y, v2->z + 4));
            }
        }

        if ((car.front->var_5F & flags_5f::breakdown_pending) && !isTitleMode())
        {
            auto newConfig = config::getNew();
            if (!newConfig.breakdowns_disabled)
            {
                car.front->var_5F &= ~flags_5f::breakdown_pending;
                car.front->var_5F |= flags_5f::broken_down;
                car.front->var_6A = 5;
                sub_4BAA76();

                auto v2 = car.body;
                auto soundId = (audio::sound_id)gPrng().randNext(26, 26 + 5);
                audio::playSound(soundId, loc16(v2->x, v2->y, v2->z + 22));
            }
        }
    }
}

void vehicle_head::sub_4BAA76()
{
    registers regs;
    regs.esi = (int32_t)this;
    call(0x004BAA76, regs);
}

static uint16_t sub_4BE368(uint32_t distance);

// 0x004AA1D0
int32_t openloco::vehicle_body::update()
{
    registers regs;
    regs.esi = (int32_t)this;

    if (mode == TransportMode::air || mode == TransportMode::water)
    {
        animationUpdate();
        return 0;
    }

    if (vehicle_var_1136237 | vehicle_var_1136238)
    {
        invalidateSprite();
        sub_4AC255(vehicleUpdate_backBogie, vehicleUpdate_frontBogie);
        invalidateSprite();
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
    animationUpdate();
    sub_4AAB0B();
    vehicle_var_1136130 = backup1136130;
    return 0;
}

// 0x004AAC4E
void openloco::vehicle_body::animationUpdate()
{
    if (var_38 & things::vehicle::flags_38::unk_4)
        return;

    vehicle_head* headVeh = vehicleUpdate_head;
    if ((headVeh->var_5D == 8) || (headVeh->var_5D == 9))
        return;

    auto vehicleObject = object();
    int32_t var_05 = vehicleObject->var_24[body_index].var_05;
    if (var_05 == 0)
    {
        return;
    }

    var_05 -= 0x80;

    switch (vehicleObject->animation[0].type)
    {
        case simple_animation_type::none:
            break;
        case simple_animation_type::steam_puff1:
        case simple_animation_type::steam_puff2:
        case simple_animation_type::steam_puff3:
            steamPuffsAnimationUpdate(0, var_05);
            break;
        case simple_animation_type::diesel_exhaust1:
            dieselExhaust1AnimationUpdate(0, var_05);
            break;
        case simple_animation_type::electric_spark1:
            electricSpark1AnimationUpdate(0, var_05);
            break;
        case simple_animation_type::electric_spark2:
            electricSpark2AnimationUpdate(0, var_05);
            break;
        case simple_animation_type::diesel_exhaust2:
            dieselExhaust2AnimationUpdate(0, var_05);
            break;
        case simple_animation_type::ship_wake:
            shipWakeAnimationUpdate(0, var_05);
            break;
        default:
            assert(false);
            break;
    }
    secondaryAnimationUpdate();
}

// 0x004AAB0B
void openloco::vehicle_body::sub_4AAB0B()
{
    int32_t eax = vehicle_var_1136130 >> 3;
    if (var_38 & things::vehicle::flags_38::unk_1)
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
        vehicle_2* veh3 = vehicleUpdate_2;
        al = (veh3->var_56 >> 16) / (vehicle_object->speed / vehicle_object->sprites[object_sprite_type].var_02);
        al = std::min(al, vehicle_object->sprites[object_sprite_type].var_02);
    }
    else if (vehicle_object->sprites[object_sprite_type].var_05 != 1)
    {
        vehicle_bogie* frontBogie = vehicleUpdate_frontBogie;
        vehicle_2* veh3 = vehicleUpdate_2;
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
                if (var_38 & things::vehicle::flags_38::unk_1)
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
                if (var_38 & things::vehicle::flags_38::unk_1)
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
        invalidateSprite();
    }
}

// 0x004AC255
void openloco::vehicle_body::sub_4AC255(vehicle_bogie* back_bogie, vehicle_bogie* front_bogie)
{
    loc16 loc = {
        static_cast<int16_t>((front_bogie->x + back_bogie->x) / 2),
        static_cast<int16_t>((front_bogie->y + back_bogie->y) / 2),
        static_cast<int16_t>((front_bogie->z + back_bogie->z) / 2)
    };
    moveTo(loc);

    if (object_sprite_type == 0xFF)
        return;

    auto distance_x = front_bogie->x - back_bogie->x;
    auto distance_y = front_bogie->y - back_bogie->y;

    auto offset = sub_4BE368(distance_x * distance_x + distance_y * distance_y);

    auto vehicle_object = object();

    if (vehicle_object->sprites[object_sprite_type].flags & (1 << 4))
    {
        sprite_pitch = updateSpritePitchSteepSlopes(offset, front_bogie->z - back_bogie->z);
    }
    else
    {
        sprite_pitch = updateSpritePitch(offset, front_bogie->z - back_bogie->z);
    }

    // If the sprite_pitch is odd
    if (sprite_pitch & 1)
    {
        sprite_yaw = updateSpriteYaw1(distance_x, distance_y);
    }
    else
    {
        auto sprite = vehicle_object->sprites[object_sprite_type];
        uint8_t i = sprite_pitch == 0 ? sprite.var_0B : sprite.var_0C;
        switch (i)
        {
            case 0:
                sprite_yaw = updateSpriteYaw0(distance_x, distance_y);
                break;
            case 1:
                sprite_yaw = updateSpriteYaw1(distance_x, distance_y);
                break;
            case 2:
                sprite_yaw = updateSpriteYaw2(distance_x, distance_y);
                break;
            case 3:
                sprite_yaw = updateSpriteYaw3(distance_x, distance_y);
                break;
            case 4:
                sprite_yaw = updateSpriteYaw4(distance_x, distance_y);
                break;
        }
    }
}

// 0x004BE368
static uint16_t sub_4BE368(uint32_t distance)
{
    uint8_t i = 10;
    for (; distance > 4096; --i, distance >>= 2)
        ;

    return vehicle_arr_500B50[distance >> 1] >> i;
}

// 0x004BF4DA
uint8_t openloco::vehicle_body::updateSpritePitchSteepSlopes(uint16_t xy_offset, int16_t z_offset)
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
uint8_t openloco::vehicle_body::updateSpritePitch(uint16_t xy_offset, int16_t z_offset)
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
uint8_t openloco::vehicle_body::updateSpriteYaw0(int16_t x_offset, int16_t y_offset)
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
uint8_t openloco::vehicle_body::updateSpriteYaw1(int16_t x_offset, int16_t y_offset)
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
uint8_t openloco::vehicle_body::updateSpriteYaw2(int16_t x_offset, int16_t y_offset)
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
uint8_t openloco::vehicle_body::updateSpriteYaw3(int16_t x_offset, int16_t y_offset)
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
uint8_t openloco::vehicle_body::updateSpriteYaw4(int16_t x_offset, int16_t y_offset)
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
void openloco::vehicle_body::secondaryAnimationUpdate()
{
    auto vehicleObject = object();

    uint8_t var_05 = vehicleObject->var_24[body_index].var_05;
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
            steamPuffsAnimationUpdate(1, var_05);
            break;
        case simple_animation_type::diesel_exhaust1:
            dieselExhaust1AnimationUpdate(1, var_05);
            break;
        case simple_animation_type::electric_spark1:
            electricSpark1AnimationUpdate(1, var_05);
            break;
        case simple_animation_type::electric_spark2:
            electricSpark2AnimationUpdate(1, var_05);
            break;
        case simple_animation_type::diesel_exhaust2:
            dieselExhaust2AnimationUpdate(1, var_05);
            break;
        case simple_animation_type::ship_wake:
            shipWakeAnimationUpdate(1, var_05);
            break;
        default:
            assert(false);
            break;
    }
}

// 0x004AB688, 0x004AACA5
void openloco::vehicle_body::steamPuffsAnimationUpdate(uint8_t num, int32_t var_05)
{
    auto vehicleObject = object();
    vehicle_bogie* frontBogie = vehicleUpdate_frontBogie;
    vehicle_bogie* backBogie = vehicleUpdate_backBogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* veh_2 = vehicleUpdate_2;
    bool soundCode = false;
    if (veh_2->var_5A == 1 || veh_2->var_5A == 4)
    {
        soundCode = true;
    }
    bool tickCalc = true;
    if (veh_2->var_5A != 0 && veh_2->var_56 >= 65536)
    {
        tickCalc = false;
    }

    auto _var_44 = var_44;
    // Reversing
    if (var_38 & things::vehicle::flags_38::unk_1)
    {
        var_05 = -var_05;
        _var_44 = -_var_44;
    }

    if (tickCalc && (soundCode == false))
    {
        if (scenarioTicks() & 7)
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

    // Looking for a bridge? or something on top
    if (steam_obj->var_08 & (1 << 2))
    {
        auto tile = map::tilemgr::get(frontBogie->tile_x, frontBogie->tile_y);

        for (auto& el : tile)
        {
            if (itemFound && !(el.isFlag4() | el.isFlag5()))
            {
                break;
            }
            else
            {
                itemFound = false;
            }
            auto track = el.asTrack();
            if (track == nullptr)
                continue;
            if (track->baseZ() != frontBogie->tile_base_z)
                continue;
            if (track->unkZ() != loc.z)
                continue;

            if (!track->hasStationElement())
                continue;

            if (!track->isLast())
                itemFound = true;
        }
    }

    if (itemFound)
    {
        auto soundId = static_cast<sound_object_id_t>(steam_obj->var_1F[var_55 + (steam_obj->sound_effect >> 1)]);

        if (veh_2->var_56 > 983040)
            return;

        int32_t volume = 0 - (veh_2->var_56 >> 9);

        auto height = std::get<0>(map::tilemgr::getHeight(loc.x, loc.y));

        if (loc.z <= height)
        {
            volume -= 1500;
        }

        audio::playSound(audio::makeObjectSoundId(soundId), loc, volume, 22050);
    }
    else
    {
        auto soundModifier = steam_obj->sound_effect >> 1;
        if (!(steam_obj->var_08 & (1 << 2)))
        {
            soundModifier = 0;
        }
        auto underSoundId = static_cast<sound_object_id_t>(steam_obj->var_1F[soundModifier + var_55]);
        auto soundId = static_cast<sound_object_id_t>(steam_obj->var_1F[var_55]);

        if (veh_2->var_56 > 983040)
            return;

        int32_t volume = 0 - (veh_2->var_56 >> 9);

        auto height = std::get<0>(map::tilemgr::getHeight(loc.x, loc.y));

        if (loc.z <= height)
        {
            soundId = underSoundId;
            volume -= 1500;
        }

        if (volume > -400)
        {
            volume = -400;
        }

        audio::playSound(audio::makeObjectSoundId(soundId), loc, volume, 22050);
    }
}

// 0x004AB9DD & 0x004AAFFA
void openloco::vehicle_body::dieselExhaust1AnimationUpdate(uint8_t num, int32_t var_05)
{
    vehicle_bogie* frontBogie = vehicleUpdate_frontBogie;
    vehicle_bogie* backBogie = vehicleUpdate_backBogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_head* headVeh = vehicleUpdate_head;
    vehicle_2* veh_2 = vehicleUpdate_2;
    auto vehicleObject = object();

    if (headVeh->vehicleType == VehicleType::ship)
    {
        if (veh_2->var_56 == 0)
            return;

        if (var_38 & things::vehicle::flags_38::unk_1)
        {
            var_05 = -var_05;
        }

        if (scenarioTicks() & 3)
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
        if (veh_2->var_5A != 1)
            return;

        if (var_38 & things::vehicle::flags_38::unk_1)
        {
            var_05 = -var_05;
        }

        if (scenarioTicks() & 3)
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
void openloco::vehicle_body::dieselExhaust2AnimationUpdate(uint8_t num, int32_t var_05)
{
    vehicle_bogie* frontBogie = vehicleUpdate_frontBogie;
    vehicle_bogie* backBogie = vehicleUpdate_backBogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* veh_2 = vehicleUpdate_2;
    auto vehicleObject = object();

    if (veh_2->var_5A != 1)
        return;

    if (veh_2->var_56 > 917504)
        return;

    if (var_38 & things::vehicle::flags_38::unk_1)
    {
        var_05 = -var_05;
    }

    if (scenarioTicks() & 7)
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
void openloco::vehicle_body::electricSpark1AnimationUpdate(uint8_t num, int32_t var_05)
{
    vehicle_bogie* frontBogie = vehicleUpdate_frontBogie;
    vehicle_bogie* backBogie = vehicleUpdate_backBogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* veh_2 = vehicleUpdate_2;
    auto vehicleObject = object();

    if (veh_2->var_5A != 2 && veh_2->var_5A != 1)
        return;

    auto _var_44 = var_44;
    if (var_38 & things::vehicle::flags_38::unk_1)
    {
        var_05 = -var_05;
        _var_44 = -var_44;
    }

    if (((uint16_t)vehicle_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        return;

    var_05 += 64;

    if (gPrng().randNext(std::numeric_limits<uint16_t>::max()) > 819)
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
void openloco::vehicle_body::electricSpark2AnimationUpdate(uint8_t num, int32_t var_05)
{
    vehicle_bogie* frontBogie = vehicleUpdate_frontBogie;
    vehicle_bogie* backBogie = vehicleUpdate_backBogie;
    if (frontBogie->var_5F & flags_5f::broken_down)
        return;

    vehicle_2* veh_2 = vehicleUpdate_2;
    auto vehicleObject = object();

    if (veh_2->var_5A != 2 && veh_2->var_5A != 1)
        return;

    auto _var_44 = var_44;
    if (var_38 & things::vehicle::flags_38::unk_1)
    {
        var_05 = -var_05;
        _var_44 = -var_44;
    }

    if (((uint16_t)vehicle_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        return;

    var_05 += 64;

    if (gPrng().randNext(std::numeric_limits<uint16_t>::max()) > 936)
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
    auto firstBogie = var_38 & things::vehicle::flags_38::unk_1 ? backBogie : frontBogie;
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
void openloco::vehicle_body::shipWakeAnimationUpdate(uint8_t num, int32_t)
{
    vehicle_2* veh_2 = vehicleUpdate_2;
    auto vehicleObject = object();

    if (veh_2->var_5A == 0)
        return;

    if (veh_2->var_56 < 393216)
        return;

    auto frequency = 32;
    if (veh_2->var_56 >= 589824)
    {
        frequency = 16;
        if (veh_2->var_56 >= 851968)
        {
            frequency = 8;
            if (veh_2->var_56 >= 1638400)
            {
                frequency = 4;
            }
        }
    }

    if ((scenarioTicks() % frequency) != 0)
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

// 0x004B90F0
// eax : newVehicleTypeId
// ebx : sourceVehicleTypeId;
static bool sub_4B90F0(const uint16_t newVehicleTypeId, const uint16_t sourceVehicleTypeId)
{
    auto newObject = objectmgr::get<vehicle_object>(newVehicleTypeId);       //edi
    auto sourceObject = objectmgr::get<vehicle_object>(sourceVehicleTypeId); // esi

    if ((newObject->flags & flags_E0::can_couple) && (sourceObject->flags & flags_E0::can_couple))
    {
        gGameCommandErrorText = string_ids::incompatible_vehicle;
        return false;
    }

    if (newVehicleTypeId == sourceVehicleTypeId)
    {
        return true;
    }

    for (auto i = 0; i < newObject->num_compat; ++i)
    {
        if (newObject->compatible_vehicles[i] == sourceVehicleTypeId)
        {
            return true;
        }
    }

    if (sourceObject->num_compat != 0)
    {
        for (auto i = 0; i < sourceObject->num_compat; ++i)
        {
            if (sourceObject->compatible_vehicles[i] == newVehicleTypeId)
            {
                return true;
            }
        }
    }

    if ((newObject->num_compat != 0) || (sourceObject->num_compat != 0))
    {
        gGameCommandErrorText = string_ids::incompatible_vehicle;
        return false;
    }

    return true;
}

// 0x004B9780
// used by road vehicles only maybe??
static uint32_t getVehicleTypeLength(const uint16_t vehicleTypeId)
{
    auto vehObject = objectmgr::get<vehicle_object>(vehicleTypeId);
    auto length = 0;
    for (auto i = 0; i < vehObject->var_04; ++i)
    {
        if (vehObject->var_24[i].body_sprite_ind == 0xFF)
        {
            continue;
        }

        auto unk = vehObject->var_24[i].body_sprite_ind & 0x7F;
        length += vehObject->sprites[unk].bogey_position * 2;
    }
    return length;
}

// 0x004B97B7
// used by road vehicles only maybe??
uint32_t vehicle_head::getVehicleTotalLength() // TODO: const
{
    auto totalLength = 0;
    things::vehicle::Vehicle train(this);
    for (const auto& car : train.cars)
    {
        totalLength += getVehicleTypeLength(car.body->object_id);
    }
    return totalLength;
}

// 0x004B8FA2
// esi : self
// ax  : vehicleTypeId
bool vehicle_head::isVehicleTypeCompatible(const uint16_t vehicleTypeId) // TODO: const
{
    auto newObject = objectmgr::get<vehicle_object>(vehicleTypeId);
    if (newObject->mode == TransportMode::air || newObject->mode == TransportMode::water)
    {
        things::vehicle::Vehicle train(this);
        if (!train.cars.empty())
        {
            gGameCommandErrorText = string_ids::incompatible_vehicle;
            return false;
        }
    }
    else
    {
        if (newObject->track_type != track_type)
        {
            gGameCommandErrorText = string_ids::incompatible_vehicle;
            return false;
        }
    }

    if (newObject->mode != mode)
    {
        gGameCommandErrorText = string_ids::incompatible_vehicle;
        return false;
    }

    if (newObject->type != vehicleType)
    {
        gGameCommandErrorText = string_ids::incompatible_vehicle;
        return false;
    }

    {
        things::vehicle::Vehicle train(this);
        for (const auto& car : train.cars)
        {
            // The object_id is the same for all vehicle components and car components of a car
            if (!sub_4B90F0(vehicleTypeId, car.front->object_id))
            {
                return false;
            }
        }
    }
    if (mode != TransportMode::road)
    {
        return true;
    }

    if (track_type != 0xFF)
    {
        return true;
    }

    auto curTotalLength = getVehicleTotalLength();
    auto additionalNewLength = getVehicleTypeLength(vehicleTypeId);
    if (curTotalLength + additionalNewLength > openloco::things::vehicle::max_vehicle_length)
    {
        gGameCommandErrorText = string_ids::vehicle_too_long;
        return false;
    }
    return true;
}

namespace openloco::things::vehicle
{
    Vehicle::Vehicle(uint16_t _head)
    {
        auto component = thingmgr::get<openloco::vehicle>(_head);
        head = component->asVehicleHead();
        component = component->nextVehicleComponent();
        veh1 = component->asVehicle1();
        component = component->nextVehicleComponent();
        veh2 = component->asVehicle2();
        component = component->nextVehicleComponent();
        if (component->type != VehicleThingType::tail)
        {
            cars = Cars{ Car{ component } };
        }
        while (component->type != VehicleThingType::tail)
        {
            component = component->nextVehicleComponent();
        }
        tail = component->asVehicleTail();
    }
}
