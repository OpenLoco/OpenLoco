#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Misc.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Math/Trigonometry.hpp"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "Vehicle.h"
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{

    static loco_global<VehicleHead*, 0x01136118> vehicleUpdate_head;
    static loco_global<Vehicle2*, 0x01136120> vehicleUpdate_2;
    static loco_global<VehicleBogie*, 0x01136124> vehicleUpdate_frontBogie;
    static loco_global<VehicleBogie*, 0x01136128> vehicleUpdate_backBogie;
    static loco_global<int32_t, 0x01136130> vehicleUpdate_var_1136130; // Speed
    static loco_global<uint8_t, 0x01136237> vehicle_var_1136237;       // var_28 related?
    static loco_global<uint8_t, 0x01136238> vehicle_var_1136238;       // var_28 related?
    static loco_global<int8_t[88], 0x004F865C> vehicle_arr_4F865C;     // var_2C related?
    static loco_global<bool[44], 0x004F8A7C> trackIdToSparkDirection;  // bools true for right false for left
    static loco_global<bool, 0x00525FAE> trafficHandedness;            // boolean true for right false for left

    // 0x00503E5C
    static constexpr Pitch vehicleBodyIndexToPitch[] = {
        Pitch::flat,
        Pitch::up6deg,
        Pitch::up12deg,
        Pitch::up18deg,
        Pitch::up25deg,
        Pitch::flat, // Not a straight number count
        Pitch::down6deg,
        Pitch::down12deg,
        Pitch::down18deg,
        Pitch::down25deg,
    };

    VehicleObject* VehicleBody::object() const
    {
        return ObjectManager::get<VehicleObject>(object_id);
    }

    // 0x004AA1D0
    int32_t VehicleBody::update()
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
        uint32_t backup1136130 = vehicleUpdate_var_1136130;
        if (var_5E != 0)
        {
            int32_t var_1136130 = var_5E;
            if (var_5E > 32)
            {
                var_1136130 = 64 - var_1136130;
            }

            vehicleUpdate_var_1136130 += var_1136130 * 320 + 500;
        }
        animationUpdate();
        sub_4AAB0B();
        vehicleUpdate_var_1136130 = backup1136130;
        return 0;
    }

    // 0x004AAC4E
    void VehicleBody::animationUpdate()
    {
        if (var_38 & Flags38::isGhost)
            return;

        VehicleHead* headVeh = vehicleUpdate_head;
        if ((headVeh->status == Status::crashed) || (headVeh->status == Status::stuck))
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
    void VehicleBody::sub_4AAB0B()
    {
        int32_t eax = vehicleUpdate_var_1136130 >> 3;
        if (var_38 & Flags38::isReversed)
        {
            eax = -eax;
        }

        var_44 += eax & 0xFFFF;
        if (object_sprite_type == 0xFF)
            return;

        auto vehicleObj = object();
        uint8_t al = 0;
        if (vehicleObj->bodySprites[object_sprite_type].flags & BodySpriteFlags::hasSpeedAnimation)
        {
            Vehicle2* veh3 = vehicleUpdate_2;
            al = veh3->currentSpeed / (vehicleObj->speed / vehicleObj->bodySprites[object_sprite_type].numAnimationFrames);
            al = std::min<uint8_t>(al, vehicleObj->bodySprites[object_sprite_type].numAnimationFrames - 1);
        }
        else if (vehicleObj->bodySprites[object_sprite_type].numRollFrames != 1)
        {
            VehicleBogie* frontBogie = vehicleUpdate_frontBogie;
            Vehicle2* veh3 = vehicleUpdate_2;
            al = var_46;
            int8_t ah = 0;
            if (veh3->currentSpeed < 35.0_mph)
            {
                ah = 0;
            }
            else
            {
                ah = vehicle_arr_4F865C[frontBogie->var_2C.track._data >> 2];
                if ((frontBogie->var_2C.track.id() == 12) || (frontBogie->var_2C.track.id() == 13))
                {
                    if (frontBogie->var_2E >= 48)
                    {
                        ah = -ah;
                    }
                }

                if (ah < 0)
                {
                    if (var_38 & Flags38::isReversed)
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
                    if (var_38 & Flags38::isReversed)
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
            al = (var_44 >> 12) & (vehicleObj->bodySprites[object_sprite_type].numAnimationFrames - 1);
        }
        if (var_46 != al)
        {
            var_46 = al;
            invalidateSprite();
        }
    }

    static uint8_t calculateYaw0FromVector(int16_t xDiff, int16_t yDiff);
    static uint8_t calculateYaw1FromVector(int16_t xDiff, int16_t yDiff);
    static uint8_t calculateYaw2FromVector(int16_t xDiff, int16_t yDiff);
    static uint8_t calculateYaw3FromVector(int16_t xDiff, int16_t yDiff);

    // 0x004AC255
    void VehicleBody::sub_4AC255(VehicleBogie* back_bogie, VehicleBogie* front_bogie)
    {
        auto midPoint = (front_bogie->position + back_bogie->position) / 2;
        moveTo(midPoint);

        if (object_sprite_type == 0xFF)
            return;

        auto bogieDifference = front_bogie->position - back_bogie->position;
        auto distanceBetweenBogies = Math::Vector::distance(front_bogie->position, back_bogie->position);
        auto vehObj = object();
        if (vehObj->bodySprites[object_sprite_type].flags & BodySpriteFlags::hasSteepSprites)
        {
            sprite_pitch = updateSpritePitchSteepSlopes(distanceBetweenBogies, bogieDifference.z);
        }
        else
        {
            sprite_pitch = updateSpritePitch(distanceBetweenBogies, bogieDifference.z);
        }

        // If the sprite_pitch is a transition there is always 4 bits for yaw
        if (static_cast<uint8_t>(sprite_pitch) & 1)
        {
            sprite_yaw = calculateYaw1FromVector(bogieDifference.x, bogieDifference.y);
        }
        else
        {
            auto sprite = vehObj->bodySprites[object_sprite_type];
            uint8_t i = sprite_pitch == Pitch::flat ? sprite.var_0B : sprite.var_0C;
            switch (i)
            {
                case 0:
                    sprite_yaw = calculateYaw0FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 1:
                    sprite_yaw = calculateYaw1FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 2:
                    sprite_yaw = calculateYaw2FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 3:
                    sprite_yaw = calculateYaw3FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 4:
                    sprite_yaw = calculateYaw4FromVector(bogieDifference.x, bogieDifference.y);
                    break;
            }
        }
    }

    // 0x004BF4DA
    Pitch VehicleBody::updateSpritePitchSteepSlopes(uint16_t xy_offset, int16_t z_offset)
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
            xyz = (static_cast<uint64_t>(z_offset) << 16) / xy_offset;
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
    Pitch VehicleBody::updateSpritePitch(uint16_t xy_offset, int16_t z_offset)
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
            xyz = (static_cast<uint64_t>(z_offset) << 16) / xy_offset;
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

    // 0x004BF52B For yaw of 3 bits
    static uint8_t calculateYaw0FromVector(int16_t xDiff, int16_t yDiff)
    {
        uint32_t i = 0;

        if (xDiff < 0)
        {
            i += 2;
            xDiff = -xDiff;
        }

        if (yDiff < 0)
        {
            i += 4;
            yDiff = -yDiff;
        }

        uint32_t xy = std::numeric_limits<uint32_t>::max();
        if (yDiff != 0)
        {
            xy = (static_cast<uint64_t>(xDiff) << 16) / yDiff;
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

    // 0x004BF56B For yaw of 3 bits (special plane version)
    uint8_t calculateYaw1FromVectorPlane(int16_t xDiff, int16_t yDiff)
    {
        uint32_t i = 0;

        if (xDiff < 0)
        {
            i += 3;
            xDiff = -xDiff;
        }

        if (yDiff < 0)
        {
            i += 6;
            yDiff = -yDiff;
        }

        uint32_t xy = std::numeric_limits<uint32_t>::max();
        if (yDiff != 0)
        {
            xy = (static_cast<uint64_t>(xDiff) << 16) / yDiff;
        }

        if (xy >= 3434)
        {
            i++;
            if (xy >= 1250501)
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

    // 0x004BF5B3 For yaw of 4 bits
    static uint8_t calculateYaw1FromVector(int16_t xDiff, int16_t yDiff)
    {
        uint32_t i = 0;

        if (xDiff < 0)
        {
            i += 3;
            xDiff = -xDiff;
        }

        if (yDiff < 0)
        {
            i += 6;
            yDiff = -yDiff;
        }

        uint32_t xy = std::numeric_limits<uint32_t>::max();
        if (yDiff != 0)
        {
            xy = (static_cast<uint64_t>(xDiff) << 16) / yDiff;
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

    // 0x004BF5FB For yaw of 5 bits
    static uint8_t calculateYaw2FromVector(int16_t xDiff, int16_t yDiff)
    {
        uint32_t i = 0;

        if (xDiff < 0)
        {
            i += 5;
            xDiff = -xDiff;
        }

        if (yDiff < 0)
        {
            i += 10;
            yDiff = -yDiff;
        }

        uint32_t xy = std::numeric_limits<uint32_t>::max();
        if (yDiff != 0)
        {
            xy = (static_cast<uint64_t>(xDiff) << 16) / yDiff;
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

    // 0x004BF657 For yaw of 6 bits
    static uint8_t calculateYaw3FromVector(int16_t xDiff, int16_t yDiff)
    {
        uint32_t i = 0;

        if (xDiff < 0)
        {
            i += 9;
            xDiff = -xDiff;
        }

        if (yDiff < 0)
        {
            i += 18;
            yDiff = -yDiff;
        }

        uint32_t xy = std::numeric_limits<uint32_t>::max();
        if (yDiff != 0)
        {
            xy = (static_cast<uint64_t>(xDiff) << 16) / yDiff;
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

    // 0x004BF6DF For yaw of 7 bits
    uint8_t calculateYaw4FromVector(int16_t xDiff, int16_t yDiff)
    {
        uint32_t i = 0;

        if (xDiff < 0)
        {
            i += 17;
            xDiff = -xDiff;
        }

        if (yDiff < 0)
        {
            i += 34;
            yDiff = -yDiff;
        }

        uint32_t xy = std::numeric_limits<uint32_t>::max();
        if (yDiff != 0)
        {
            xy = (static_cast<uint64_t>(xDiff) << 16) / yDiff;
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
    void VehicleBody::secondaryAnimationUpdate()
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
    void VehicleBody::steamPuffsAnimationUpdate(uint8_t num, int32_t var_05)
    {
        auto vehicleObject = object();
        VehicleBogie* frontBogie = vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = vehicleUpdate_backBogie;
        if (frontBogie->var_5F & Flags5F::broken_down)
            return;

        Vehicle2* veh_2 = vehicleUpdate_2;
        bool soundCode = false;
        if (veh_2->var_5A == 1 || veh_2->var_5A == 4)
        {
            soundCode = true;
        }
        bool tickCalc = true;
        if (veh_2->var_5A != 0 && veh_2->currentSpeed >= 1.0_mph)
        {
            tickCalc = false;
        }

        auto _var_44 = var_44;
        // Reversing
        if (var_38 & Flags38::isReversed)
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
            if (vehicleUpdate_var_1136130 + (uint16_t)(_var_44 * 8) < std::numeric_limits<uint16_t>::max())
            {
                return;
            }
        }

        var_05 += 64;

        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].height, sprite_pitch, sprite_yaw);

        auto bogieDifference = backBogie->position - frontBogie->position;

        auto smokeLoc = bogieDifference * var_05 / 128 + frontBogie->position + Map::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].height);

        Exhaust::create(smokeLoc, vehicleObject->animation[num].object_id | (soundCode ? 0 : 0x80));
        if (soundCode == false)
            return;

        var_55++;
        SteamObject* steam_obj = ObjectManager::get<SteamObject>(vehicleObject->animation[num].object_id);
        if (var_55 >= ((uint8_t)vehicleObject->animation[num].type) + 1)
        {
            var_55 = 0;
        }

        bool stationFound = false;

        // Looking for a station
        if (steam_obj->var_08 & (1 << 2))
        {
            auto tile = Map::TileManager::get(frontBogie->tile_x, frontBogie->tile_y);

            for (auto& el : tile)
            {
                if (stationFound && !(el.isGhost() || el.isFlag5()))
                {
                    break;
                }
                else
                {
                    stationFound = false;
                }
                auto track = el.asTrack();
                if (track == nullptr)
                    continue;
                if (track->baseZ() != frontBogie->tile_base_z)
                    continue;
                if (track->trackId() != frontBogie->var_2C.track.id())
                    continue;
                if (track->unkDirection() != frontBogie->var_2C.track.cardinalDirection())
                    continue;
                if (!track->hasStationElement())
                    continue;

                if (!track->isLast())
                    stationFound = true;
            }
        }

        if (stationFound)
        {
            auto soundId = static_cast<SoundObjectId_t>(steam_obj->var_1F[var_55 + (steam_obj->sound_effect >> 1)]);

            if (veh_2->currentSpeed > 15.0_mph)
                return;

            int32_t volume = 0 - (veh_2->currentSpeed.getRaw() >> 9);

            auto height = Map::TileManager::getHeight(smokeLoc).landHeight;

            if (smokeLoc.z <= height)
            {
                volume -= 1500;
            }

            Audio::playSound(Audio::makeObjectSoundId(soundId), smokeLoc, volume, 22050);
        }
        else
        {
            auto soundModifier = steam_obj->sound_effect >> 1;
            if (!(steam_obj->var_08 & (1 << 2)))
            {
                soundModifier = 0;
            }
            auto underSoundId = static_cast<SoundObjectId_t>(steam_obj->var_1F[soundModifier + var_55]);
            auto soundId = static_cast<SoundObjectId_t>(steam_obj->var_1F[var_55]);

            if (veh_2->currentSpeed > 15.0_mph)
                return;

            int32_t volume = 0 - (veh_2->currentSpeed.getRaw() >> 9);

            auto height = Map::TileManager::getHeight(smokeLoc).landHeight;

            if (smokeLoc.z <= height)
            {
                soundId = underSoundId;
                volume -= 1500;
            }

            if (volume > -400)
            {
                volume = -400;
            }

            Audio::playSound(Audio::makeObjectSoundId(soundId), smokeLoc, volume, 22050);
        }
    }

    // 0x004AB9DD & 0x004AAFFA
    void VehicleBody::dieselExhaust1AnimationUpdate(uint8_t num, int32_t var_05)
    {
        VehicleBogie* frontBogie = vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = vehicleUpdate_backBogie;
        if (frontBogie->var_5F & Flags5F::broken_down)
            return;

        VehicleHead* headVeh = vehicleUpdate_head;
        Vehicle2* veh_2 = vehicleUpdate_2;
        auto vehicleObject = object();

        if (headVeh->vehicleType == VehicleType::ship)
        {
            if (veh_2->currentSpeed == 0.0_mph)
                return;

            if (var_38 & Flags38::isReversed)
            {
                var_05 = -var_05;
            }

            if (scenarioTicks() & 3)
                return;

            auto positionFactor = vehicleObject->bodySprites[0].bogey_position * var_05 / 256;
            auto invertedDirection = sprite_yaw ^ (1 << 5);
            auto xyFactor = Math::Trigonometry::computeXYVector(positionFactor, invertedDirection) / 2;

            Map::Pos3 loc = position + Map::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].height);
            Exhaust::create(loc, vehicleObject->animation[num].object_id);
        }
        else
        {
            if (veh_2->var_5A != 1)
                return;

            if (var_38 & Flags38::isReversed)
            {
                var_05 = -var_05;
            }

            if (scenarioTicks() & 3)
                return;

            if (var_5E != 0)
                return;

            var_05 += 64;
            auto bogieDifference = backBogie->position - frontBogie->position;
            auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].height, sprite_pitch, sprite_yaw);

            auto loc = bogieDifference * var_05 / 128 + frontBogie->position + Map::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].height);

            Exhaust::create(loc, vehicleObject->animation[num].object_id);
        }
    }

    // 0x004ABB5A & 0x004AB177
    void VehicleBody::dieselExhaust2AnimationUpdate(uint8_t num, int32_t var_05)
    {
        VehicleBogie* frontBogie = vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = vehicleUpdate_backBogie;
        if (frontBogie->var_5F & Flags5F::broken_down)
            return;

        Vehicle2* veh_2 = vehicleUpdate_2;
        auto vehicleObject = object();

        if (veh_2->var_5A != 1)
            return;

        if (veh_2->currentSpeed > 14.0_mph)
            return;

        if (var_38 & Flags38::isReversed)
        {
            var_05 = -var_05;
        }

        if (scenarioTicks() & 7)
            return;

        var_05 += 64;

        auto bogieDifference = backBogie->position - frontBogie->position;
        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].height, sprite_pitch, sprite_yaw);

        auto loc = bogieDifference * var_05 / 128 + frontBogie->position + Map::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].height);

        // 90 degrees C.W.
        auto yaw = (sprite_yaw + 16) & 0x3F;

        auto unkFactor = 5;
        if (trafficHandedness != 0)
        {
            unkFactor = -5;
        }

        xyFactor = Math::Trigonometry::computeXYVector(unkFactor, yaw);
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].object_id);
    }

    // 0x004ABDAD & 0x004AB3CA
    void VehicleBody::electricSpark1AnimationUpdate(uint8_t num, int32_t var_05)
    {
        VehicleBogie* frontBogie = vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = vehicleUpdate_backBogie;
        if (frontBogie->var_5F & Flags5F::broken_down)
            return;

        Vehicle2* veh_2 = vehicleUpdate_2;
        auto vehicleObject = object();

        if (veh_2->var_5A != 2 && veh_2->var_5A != 1)
            return;

        auto _var_44 = var_44;
        if (var_38 & Flags38::isReversed)
        {
            var_05 = -var_05;
            _var_44 = -var_44;
        }

        if (((uint16_t)vehicleUpdate_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
            return;

        var_05 += 64;

        if (gPrng().randNext(std::numeric_limits<uint16_t>::max()) > 819)
            return;

        auto bogieDifference = backBogie->position - frontBogie->position;
        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].height, sprite_pitch, sprite_yaw);

        auto loc = bogieDifference * var_05 / 128 + frontBogie->position + Map::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].height);

        Exhaust::create(loc, vehicleObject->animation[num].object_id);
    }

    // 0x004ABEC3 & 0x004AB4E0
    void VehicleBody::electricSpark2AnimationUpdate(uint8_t num, int32_t var_05)
    {
        VehicleBogie* frontBogie = vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = vehicleUpdate_backBogie;
        if (frontBogie->var_5F & Flags5F::broken_down)
            return;

        Vehicle2* veh_2 = vehicleUpdate_2;
        auto vehicleObject = object();

        if (veh_2->var_5A != 2 && veh_2->var_5A != 1)
            return;

        auto _var_44 = var_44;
        if (var_38 & Flags38::isReversed)
        {
            var_05 = -var_05;
            _var_44 = -var_44;
        }

        if (((uint16_t)vehicleUpdate_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
            return;

        var_05 += 64;

        if (gPrng().randNext(std::numeric_limits<uint16_t>::max()) > 936)
            return;

        auto bogieDifference = backBogie->position - frontBogie->position;
        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].height, sprite_pitch, sprite_yaw);

        auto loc = bogieDifference * var_05 / 128 + frontBogie->position + Map::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].height);

        // 90 degrees C.W.
        auto yaw = (sprite_yaw + 16) & 0x3F;
        auto firstBogie = var_38 & Flags38::isReversed ? backBogie : frontBogie;
        auto unkFactor = 5;
        if (!trackIdToSparkDirection[(firstBogie->var_2C.road._data >> 3)])
        {
            unkFactor = -5;
        }

        if (firstBogie->var_2C.road.isRightLane())
        {
            unkFactor = -unkFactor;
        }

        xyFactor = Math::Trigonometry::computeXYVector(unkFactor, yaw);
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].object_id);
    }

    // 0x004ABC8A & 0x004AB2A7
    void VehicleBody::shipWakeAnimationUpdate(uint8_t num, int32_t)
    {
        Vehicle2* veh_2 = vehicleUpdate_2;
        auto vehicleObject = object();

        if (veh_2->var_5A == 0)
            return;

        if (veh_2->currentSpeed < 6.0_mph)
            return;

        auto frequency = 32;
        if (veh_2->currentSpeed >= 9.0_mph)
        {
            frequency = 16;
            if (veh_2->currentSpeed >= 13.0_mph)
            {
                frequency = 8;
                if (veh_2->currentSpeed >= 25.0_mph)
                {
                    frequency = 4;
                }
            }
        }

        if ((scenarioTicks() % frequency) != 0)
            return;

        auto positionFactor = vehicleObject->bodySprites[0].bogey_position;
        auto invertedDirection = sprite_yaw ^ (1 << 5);
        auto xyFactor = Math::Trigonometry::computeXYVector(positionFactor, invertedDirection) / 4;

        Map::Pos3 loc = position + Map::Pos3(xyFactor.x, xyFactor.y, 0);

        // 90 degrees C.W.
        auto yaw = (sprite_yaw + 16) & 0x3F;

        xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->var_113, yaw) / 2;
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].object_id);

        if (vehicleObject->var_113 == 0)
            return;

        // 90 degrees C.C.W.
        yaw = (sprite_yaw - 16) & 0x3F;

        xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->var_113, yaw) / 2;
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].object_id);
    }

    // 0x004AC039
    void VehicleBody::updateCargoSprite()
    {
        if (object_sprite_type == 0xFF)
        {
            return;
        }
        if (primaryCargo.maxQty == 0)
        {
            return;
        }

        auto vehicleObj = object();
        auto& bodySprite = vehicleObj->bodySprites[object_sprite_type];

        auto percentageFull = std::min((primaryCargo.qty * 256) / primaryCargo.maxQty, 255);
        auto spriteIndex = (percentageFull * bodySprite.numCargoLoadFrames) / 256;
        if (spriteIndex != 0)
        {
            spriteIndex += vehicleObj->cargoTypeSpriteOffsets[primaryCargo.type];
        }
        spriteIndex *= bodySprite.numAnimationFrames;
        if (spriteIndex != var_47)
        {
            var_47 = spriteIndex;
            invalidateSprite();
        }
    }
}
