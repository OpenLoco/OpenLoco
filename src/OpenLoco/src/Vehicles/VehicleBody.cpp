#include "Audio/Audio.h"
#include "Config.h"
#include "Effects/Effect.h"
#include "Effects/ExhaustEffect.h"
#include "Entities/EntityManager.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/SteamObject.h"
#include "Objects/VehicleObject.h"
#include "Random.h"
#include "ScenarioManager.h"
#include "Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Trigonometry.hpp>
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{

    static loco_global<VehicleHead*, 0x01136118> _vehicleUpdate_head;
    static loco_global<Vehicle2*, 0x01136120> _vehicleUpdate_2;
    static loco_global<VehicleBogie*, 0x01136124> _vehicleUpdate_frontBogie;
    static loco_global<VehicleBogie*, 0x01136128> _vehicleUpdate_backBogie;
    static loco_global<int32_t, 0x01136130> _vehicleUpdate_var_1136130; // Speed
    static loco_global<bool, 0x01136237> _vehicleUpdate_frontBogieHasMoved;
    static loco_global<bool, 0x01136238> _vehicleUpdate_backBogieHasMoved;
    static loco_global<int8_t[88], 0x004F865C> _vehicle_arr_4F865C; // cargoType related?

    // 0x00503E5C
    static constexpr Pitch kVehicleBodyIndexToPitch[] = {
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

    const VehicleObject* VehicleBody::getObject() const
    {
        return ObjectManager::get<VehicleObject>(objectId);
    }

    // 0x004AA1D0
    bool VehicleBody::update()
    {
        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            animationUpdate();
            return true;
        }

        if (_vehicleUpdate_frontBogieHasMoved || _vehicleUpdate_backBogieHasMoved)
        {
            invalidateSprite();
            sub_4AC255(_vehicleUpdate_backBogie, _vehicleUpdate_frontBogie);
            invalidateSprite();
        }
        uint32_t backup1136130 = _vehicleUpdate_var_1136130;
        if (wheelSlipping != 0)
        {
            int32_t var_1136130 = wheelSlipping;
            if (wheelSlipping > kWheelSlippingDuration / 2)
            {
                var_1136130 = kWheelSlippingDuration - var_1136130;
            }

            _vehicleUpdate_var_1136130 += var_1136130 * 320 + 500;
        }
        animationUpdate();
        sub_4AAB0B();
        _vehicleUpdate_var_1136130 = backup1136130;
        return true;
    }

    // 0x004AAC4E
    void VehicleBody::animationUpdate()
    {
        if (has38Flags(Flags38::isGhost))
        {
            return;
        }

        VehicleHead* headVeh = _vehicleUpdate_head;
        if ((headVeh->status == Status::crashed) || (headVeh->status == Status::stuck))
        {
            return;
        }

        const auto* vehicleObject = getObject();
        int32_t emitterHorizontalPos = vehicleObject->carComponents[bodyIndex].emitterHorizontalPos;
        if (emitterHorizontalPos == 0)
        {
            return;
        }

        emitterHorizontalPos -= 0x80;

        switch (vehicleObject->animation[0].type)
        {
            case SimpleAnimationType::none:
                break;
            case SimpleAnimationType::steam_puff1:
            case SimpleAnimationType::steam_puff2:
            case SimpleAnimationType::steam_puff3:
                steamPuffsAnimationUpdate(0, emitterHorizontalPos);
                break;
            case SimpleAnimationType::diesel_exhaust1:
                dieselExhaust1AnimationUpdate(0, emitterHorizontalPos);
                break;
            case SimpleAnimationType::electric_spark1:
                electricSpark1AnimationUpdate(0, emitterHorizontalPos);
                break;
            case SimpleAnimationType::electric_spark2:
                electricSpark2AnimationUpdate(0, emitterHorizontalPos);
                break;
            case SimpleAnimationType::diesel_exhaust2:
                dieselExhaust2AnimationUpdate(0, emitterHorizontalPos);
                break;
            case SimpleAnimationType::ship_wake:
                shipWakeAnimationUpdate(0, emitterHorizontalPos);
                break;
            default:
                assert(false);
                break;
        }
        secondaryAnimationUpdate();
    }

    // 0x004AA904
    void VehicleBody::updateSegmentCrashed()
    {
        invalidateSprite();
        sub_4AC255(_vehicleUpdate_backBogie, _vehicleUpdate_frontBogie);
        invalidateSprite();
        animationUpdate();
        sub_4AAB0B();
        if (!hasVehicleFlags(VehicleFlags::unk_5))
        {
            VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
            VehicleBogie* backBogie = _vehicleUpdate_backBogie;

            if (frontBogie->hasVehicleFlags(VehicleFlags::unk_5)
                || backBogie->hasVehicleFlags(VehicleFlags::unk_5))
            {
                explodeComponent();
                this->vehicleFlags |= VehicleFlags::unk_5;
            }
        }
    }

    // 0x004AAB0B
    void VehicleBody::sub_4AAB0B()
    {
        int32_t eax = _vehicleUpdate_var_1136130 >> 3;
        if (has38Flags(Flags38::isReversed))
        {
            eax = -eax;
        }

        var_44 += eax & 0xFFFF;
        if (objectSpriteType == 0xFF)
        {
            return;
        }

        const auto* vehicleObj = getObject();
        uint8_t targetAnimationFrame = 0;
        if (vehicleObj->bodySprites[objectSpriteType].hasFlags(BodySpriteFlags::hasSpeedAnimation))
        {
            Vehicle2* veh3 = _vehicleUpdate_2;
            targetAnimationFrame = veh3->currentSpeed / (vehicleObj->speed / vehicleObj->bodySprites[objectSpriteType].numAnimationFrames);
            targetAnimationFrame = std::min<uint8_t>(targetAnimationFrame, vehicleObj->bodySprites[objectSpriteType].numAnimationFrames - 1);
        }
        else if (vehicleObj->bodySprites[objectSpriteType].numRollFrames != 1)
        {
            VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
            Vehicle2* veh3 = _vehicleUpdate_2;
            targetAnimationFrame = animationFrame;
            int8_t targetTiltFrame = 0;
            if (veh3->currentSpeed < 35.0_mph)
            {
                targetTiltFrame = 0;
            }
            else
            {
                targetTiltFrame = _vehicle_arr_4F865C[frontBogie->trackAndDirection.track._data >> 2];
                // S-bend
                if ((frontBogie->trackAndDirection.track.id() == 12) || (frontBogie->trackAndDirection.track.id() == 13))
                {
                    if (frontBogie->subPosition >= 48)
                    {
                        targetTiltFrame = -targetTiltFrame;
                    }
                }

                if (targetTiltFrame < 0)
                {
                    if (has38Flags(Flags38::isReversed))
                    {
                        targetTiltFrame = 2;
                        if (targetAnimationFrame != 0 && targetAnimationFrame != targetTiltFrame)
                        {
                            targetTiltFrame = 0;
                        }
                    }
                    else
                    {
                        targetTiltFrame = 1;
                        if (targetAnimationFrame != 0 && targetAnimationFrame != targetTiltFrame)
                        {
                            targetTiltFrame = 0;
                        }
                    }
                }
                else if (targetTiltFrame > 0)
                {
                    if (has38Flags(Flags38::isReversed))
                    {
                        targetTiltFrame = 1;
                        if (targetAnimationFrame != 0 && targetAnimationFrame != targetTiltFrame)
                        {
                            targetTiltFrame = 0;
                        }
                    }
                    else
                    {
                        targetTiltFrame = 2;
                        if (targetAnimationFrame != 0 && targetAnimationFrame != targetTiltFrame)
                        {
                            targetTiltFrame = 0;
                        }
                    }
                }
                else
                {
                    targetTiltFrame = 0;
                }
            }
            targetAnimationFrame = targetTiltFrame;
        }
        else
        {
            targetAnimationFrame = (var_44 >> 12) & (vehicleObj->bodySprites[objectSpriteType].numAnimationFrames - 1);
        }
        if (animationFrame != targetAnimationFrame)
        {
            animationFrame = targetAnimationFrame;
            invalidateSprite();
        }
    }

    static uint8_t calculateYaw2FromVector(int16_t xDiff, int16_t yDiff);
    static uint8_t calculateYaw3FromVector(int16_t xDiff, int16_t yDiff);

    // 0x004AC255
    void VehicleBody::sub_4AC255(VehicleBogie* back_bogie, VehicleBogie* front_bogie)
    {
        auto midPoint = (front_bogie->position + back_bogie->position) / 2;
        moveTo(midPoint);

        if (objectSpriteType == 0xFF)
        {
            return;
        }

        auto bogieDifference = front_bogie->position - back_bogie->position;
        auto distanceBetweenBogies = Math::Vector::distance2D(front_bogie->position, back_bogie->position);
        const auto* vehObj = getObject();
        if (vehObj->bodySprites[objectSpriteType].hasFlags(BodySpriteFlags::hasSteepSprites))
        {
            spritePitch = updateSpritePitchSteepSlopes(distanceBetweenBogies, bogieDifference.z);
        }
        else
        {
            spritePitch = updateSpritePitch(distanceBetweenBogies, bogieDifference.z);
        }

        // If the sprite_pitch is a transition there is always 4 bits for yaw
        if (static_cast<uint8_t>(spritePitch) & 1)
        {
            spriteYaw = calculateYaw1FromVector(bogieDifference.x, bogieDifference.y);
        }
        else
        {
            auto sprite = vehObj->bodySprites[objectSpriteType];
            uint8_t i = spritePitch == Pitch::flat ? sprite.flatYawAccuracy : sprite.slopedYawAccuracy;
            switch (i)
            {
                case 0:
                    spriteYaw = calculateYaw0FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 1:
                    spriteYaw = calculateYaw1FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 2:
                    spriteYaw = calculateYaw2FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 3:
                    spriteYaw = calculateYaw3FromVector(bogieDifference.x, bogieDifference.y);
                    break;
                case 4:
                    spriteYaw = calculateYaw4FromVector(bogieDifference.x, bogieDifference.y);
                    break;
            }
        }
    }

    // 0x004BF4DA
    Pitch VehicleBody::updateSpritePitchSteepSlopes(uint16_t xyOffset, int16_t zOffset)
    {
        uint32_t i = 0;

        if (zOffset < 0)
        {
            i = 5;
            zOffset = -zOffset;
        }

        uint32_t xyz = std::numeric_limits<uint32_t>::max();
        if (xyOffset != 0)
        {
            xyz = (static_cast<uint64_t>(zOffset) << 16) / xyOffset;
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

        return kVehicleBodyIndexToPitch[i];
    }

    // 0x004BF49D
    Pitch VehicleBody::updateSpritePitch(uint16_t xyOffset, int16_t zOffset)
    {
        uint32_t i = 0;

        if (zOffset < 0)
        {
            i = 5;
            zOffset = -zOffset;
        }

        uint32_t xyz = std::numeric_limits<uint32_t>::max();
        if (xyOffset != 0)
        {
            xyz = (static_cast<uint64_t>(zOffset) << 16) / xyOffset;
        }

        if (xyz >= 3331)
        {
            i++;
            if (xyz >= 9000)
            {
                i++;
            }
        }

        return kVehicleBodyIndexToPitch[i];
    }

    // 0x004BF52B For yaw of 3 bits
    uint8_t calculateYaw0FromVector(int16_t xDiff, int16_t yDiff)
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
        constexpr uint8_t kIndexToYaw[] = {
            16,
            32,
            16,
            0,
            48,
            32,
            48,
            0
        };
        return kIndexToYaw[i];
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
        constexpr uint8_t kIndexToYaw[] = {
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
        return kIndexToYaw[i];
    }

    // 0x004BF5B3 For yaw of 4 bits
    uint8_t calculateYaw1FromVector(int16_t xDiff, int16_t yDiff)
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
        constexpr uint8_t kIndexToYaw[] = {
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
        return kIndexToYaw[i];
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
        constexpr uint8_t kIndexToYaw[] = {
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
        return kIndexToYaw[i];
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
        constexpr uint8_t kIndexToYaw[] = {
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
        return kIndexToYaw[i];
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
        constexpr uint8_t kIndexToYaw[] = {
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
        return kIndexToYaw[i];
    }

    // 0x004AB655
    void VehicleBody::secondaryAnimationUpdate()
    {
        const auto* vehicleObject = getObject();

        uint8_t emitterHorizontalPos = vehicleObject->carComponents[bodyIndex].emitterHorizontalPos;
        if (emitterHorizontalPos == 0)
        {
            return;
        }

        emitterHorizontalPos -= 0x80;

        switch (vehicleObject->animation[1].type)
        {
            case SimpleAnimationType::none:
                return;
            case SimpleAnimationType::steam_puff1:
            case SimpleAnimationType::steam_puff2:
            case SimpleAnimationType::steam_puff3:
                steamPuffsAnimationUpdate(1, emitterHorizontalPos);
                break;
            case SimpleAnimationType::diesel_exhaust1:
                dieselExhaust1AnimationUpdate(1, emitterHorizontalPos);
                break;
            case SimpleAnimationType::electric_spark1:
                electricSpark1AnimationUpdate(1, emitterHorizontalPos);
                break;
            case SimpleAnimationType::electric_spark2:
                electricSpark2AnimationUpdate(1, emitterHorizontalPos);
                break;
            case SimpleAnimationType::diesel_exhaust2:
                dieselExhaust2AnimationUpdate(1, emitterHorizontalPos);
                break;
            case SimpleAnimationType::ship_wake:
                shipWakeAnimationUpdate(1, emitterHorizontalPos);
                break;
            default:
                assert(false);
                break;
        }
    }

    // 0x004AB688, 0x004AACA5
    void VehicleBody::steamPuffsAnimationUpdate(uint8_t num, int32_t emitterHorizontalPos)
    {
        const auto* vehicleObject = getObject();
        VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = _vehicleUpdate_backBogie;
        if (frontBogie->hasBreakdownFlags(BreakdownFlags::brokenDown))
        {
            return;
        }

        Vehicle2* veh_2 = _vehicleUpdate_2;
        bool soundCode = false;
        if (veh_2->motorState == MotorState::accelerating || veh_2->motorState == MotorState::stoppedOnIncline)
        {
            soundCode = true;
        }
        bool tickCalc = true;
        if (veh_2->motorState != MotorState::stopped && veh_2->currentSpeed >= 1.0_mph)
        {
            tickCalc = false;
        }

        auto _var_44 = var_44;
        // Reversing
        if (has38Flags(Flags38::isReversed))
        {
            emitterHorizontalPos = -emitterHorizontalPos;
            _var_44 = -_var_44;
        }

        if (tickCalc && (soundCode == false))
        {
            if (ScenarioManager::getScenarioTicks() & 7)
            {
                return;
            }
        }
        else
        {
            if (_vehicleUpdate_var_1136130 + (uint16_t)(_var_44 * 8) < std::numeric_limits<uint16_t>::max())
            {
                return;
            }
        }

        emitterHorizontalPos += 64;

        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].emitterVerticalPos, spritePitch, spriteYaw);

        auto bogieDifference = backBogie->position - frontBogie->position;

        auto smokeLoc = bogieDifference * emitterHorizontalPos / 128 + frontBogie->position + World::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].emitterVerticalPos);

        Exhaust::create(smokeLoc, vehicleObject->animation[num].objectId | (soundCode ? 0 : 0x80));
        if (soundCode == false)
        {
            return;
        }

        chuffSoundIndex++;
        const SteamObject* steamObj = ObjectManager::get<SteamObject>(vehicleObject->animation[num].objectId);
        if (chuffSoundIndex >= ((uint8_t)vehicleObject->animation[num].type) + 1)
        {
            chuffSoundIndex = 0;
        }

        if (veh_2->currentSpeed > 15.0_mph)
        {
            return;
        }

        bool stationFound = false;

        // play tunnel sounds if the locomotive is in a station below a bridge
        if (steamObj->hasFlags(SteamObjectFlags::hasTunnelSounds))
        {
            auto tile = World::TileManager::get(frontBogie->tileX, frontBogie->tileY);

            for (auto& el : tile)
            {
                if (stationFound && !(el.isGhost() || el.isAiAllocated()))
                {
                    break;
                }
                else
                {
                    stationFound = false;
                }
                auto* track = el.as<World::TrackElement>();
                if (track == nullptr)
                {
                    continue;
                }
                if (track->baseZ() != frontBogie->tileBaseZ)
                {
                    continue;
                }
                if (track->trackId() != frontBogie->trackAndDirection.track.id())
                {
                    continue;
                }
                if (track->rotation() != frontBogie->trackAndDirection.track.cardinalDirection())
                {
                    continue;
                }
                if (!track->hasStationElement())
                {
                    continue;
                }

                if (!track->isLast())
                {
                    stationFound = true;
                }
            }
        }

        if (stationFound)
        {
            auto soundId = steamObj->soundEffects[chuffSoundIndex + (steamObj->numSoundEffects / 2)];

            int32_t volume = 0 - (veh_2->currentSpeed.getRaw() >> 9);

            auto height = World::TileManager::getHeight(smokeLoc).landHeight;

            if (smokeLoc.z <= height)
            {
                volume -= 1500;
            }

            Audio::playSound(Audio::makeObjectSoundId(soundId), smokeLoc, volume, 22050);
        }
        else
        {
            auto soundModifier = steamObj->numSoundEffects / 2;
            if (!steamObj->hasFlags(SteamObjectFlags::hasTunnelSounds))
            {
                soundModifier = 0;
            }
            auto underSoundId = steamObj->soundEffects[soundModifier + chuffSoundIndex];
            auto soundId = steamObj->soundEffects[chuffSoundIndex];

            int32_t volume = 0 - (veh_2->currentSpeed.getRaw() >> 9);

            auto height = World::TileManager::getHeight(smokeLoc).landHeight;

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
    void VehicleBody::dieselExhaust1AnimationUpdate(uint8_t num, int32_t emitterHorizontalPos)
    {
        VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = _vehicleUpdate_backBogie;
        if (frontBogie->hasBreakdownFlags(BreakdownFlags::brokenDown))
        {
            return;
        }

        VehicleHead* headVeh = _vehicleUpdate_head;
        Vehicle2* veh_2 = _vehicleUpdate_2;
        const auto* vehicleObject = getObject();

        if (headVeh->vehicleType == VehicleType::ship)
        {
            if (veh_2->currentSpeed == 0.0_mph)
            {
                return;
            }

            if (has38Flags(Flags38::isReversed))
            {
                emitterHorizontalPos = -emitterHorizontalPos;
            }

            if (ScenarioManager::getScenarioTicks() & 3)
            {
                return;
            }

            auto positionFactor = vehicleObject->bodySprites[0].halfLength * emitterHorizontalPos / 256;
            auto invertedDirection = spriteYaw ^ (1 << 5);
            auto xyFactor = Math::Trigonometry::computeXYVector(positionFactor, invertedDirection) / 2;

            World::Pos3 loc = position + World::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].emitterVerticalPos);
            Exhaust::create(loc, vehicleObject->animation[num].objectId);
        }
        else
        {
            if (veh_2->motorState != MotorState::accelerating)
            {
                return;
            }

            if (has38Flags(Flags38::isReversed))
            {
                emitterHorizontalPos = -emitterHorizontalPos;
            }

            if (ScenarioManager::getScenarioTicks() & 3)
            {
                return;
            }

            if (wheelSlipping != 0)
            {
                return;
            }

            emitterHorizontalPos += 64;
            auto bogieDifference = backBogie->position - frontBogie->position;
            auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].emitterVerticalPos, spritePitch, spriteYaw);

            auto loc = bogieDifference * emitterHorizontalPos / 128 + frontBogie->position + World::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].emitterVerticalPos);

            Exhaust::create(loc, vehicleObject->animation[num].objectId);
        }
    }

    // 0x004ABB5A & 0x004AB177
    void VehicleBody::dieselExhaust2AnimationUpdate(uint8_t num, int32_t emitterHorizontalPos)
    {
        VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = _vehicleUpdate_backBogie;
        if (frontBogie->hasBreakdownFlags(BreakdownFlags::brokenDown))
        {
            return;
        }

        Vehicle2* veh_2 = _vehicleUpdate_2;
        const auto* vehicleObject = getObject();

        if (veh_2->motorState != MotorState::accelerating)
        {
            return;
        }

        if (veh_2->currentSpeed > 14.0_mph)
        {
            return;
        }

        if (has38Flags(Flags38::isReversed))
        {
            emitterHorizontalPos = -emitterHorizontalPos;
        }

        if (ScenarioManager::getScenarioTicks() & 7)
        {
            return;
        }

        emitterHorizontalPos += 64;

        auto bogieDifference = backBogie->position - frontBogie->position;
        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].emitterVerticalPos, spritePitch, spriteYaw);

        auto loc = bogieDifference * emitterHorizontalPos / 128 + frontBogie->position + World::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].emitterVerticalPos);

        // 90 degrees C.W.
        auto yaw = (spriteYaw + 16) & 0x3F;

        auto unkFactor = 5;
        if (getGameState().trafficHandedness != 0)
        {
            unkFactor = -5;
        }

        xyFactor = Math::Trigonometry::computeXYVector(unkFactor, yaw);
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].objectId);
    }

    // 0x004ABDAD & 0x004AB3CA
    void VehicleBody::electricSpark1AnimationUpdate(uint8_t num, int32_t emitterHorizontalPos)
    {
        VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = _vehicleUpdate_backBogie;
        if (frontBogie->hasBreakdownFlags(BreakdownFlags::brokenDown))
        {
            return;
        }

        Vehicle2* veh_2 = _vehicleUpdate_2;
        const auto* vehicleObject = getObject();

        if (veh_2->motorState != MotorState::coasting && veh_2->motorState != MotorState::accelerating)
        {
            return;
        }

        auto _var_44 = var_44;
        if (has38Flags(Flags38::isReversed))
        {
            emitterHorizontalPos = -emitterHorizontalPos;
            _var_44 = -var_44;
        }

        if (((uint16_t)_vehicleUpdate_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        {
            return;
        }

        emitterHorizontalPos += 64;

        if (gPrng1().randNext(std::numeric_limits<uint16_t>::max()) > 819)
        {
            return;
        }

        auto bogieDifference = backBogie->position - frontBogie->position;
        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].emitterVerticalPos, spritePitch, spriteYaw);

        auto loc = bogieDifference * emitterHorizontalPos / 128 + frontBogie->position + World::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].emitterVerticalPos);

        Exhaust::create(loc, vehicleObject->animation[num].objectId);
    }

    // 0x004ABEC3 & 0x004AB4E0
    void VehicleBody::electricSpark2AnimationUpdate(uint8_t num, int32_t emitterHorizontalPos)
    {
        VehicleBogie* frontBogie = _vehicleUpdate_frontBogie;
        VehicleBogie* backBogie = _vehicleUpdate_backBogie;
        if (frontBogie->hasBreakdownFlags(BreakdownFlags::brokenDown))
        {
            return;
        }

        Vehicle2* veh_2 = _vehicleUpdate_2;
        const auto* vehicleObject = getObject();

        if (veh_2->motorState != MotorState::coasting && veh_2->motorState != MotorState::accelerating)
        {
            return;
        }

        auto _var_44 = var_44;
        if (has38Flags(Flags38::isReversed))
        {
            emitterHorizontalPos = -emitterHorizontalPos;
            _var_44 = -var_44;
        }

        if (((uint16_t)_vehicleUpdate_var_1136130) + ((uint16_t)_var_44 * 8) < std::numeric_limits<uint16_t>::max())
        {
            return;
        }

        emitterHorizontalPos += 64;

        if (gPrng1().randNext(std::numeric_limits<uint16_t>::max()) > 936)
        {
            return;
        }

        auto bogieDifference = backBogie->position - frontBogie->position;
        auto xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->animation[num].emitterVerticalPos, spritePitch, spriteYaw);

        auto loc = bogieDifference * emitterHorizontalPos / 128 + frontBogie->position + World::Pos3(xyFactor.x, xyFactor.y, vehicleObject->animation[num].emitterVerticalPos);

        // 90 degrees C.W.
        auto yaw = (spriteYaw + 16) & 0x3F;
        auto firstBogie = has38Flags(Flags38::isReversed) ? backBogie : frontBogie;
        auto unkFactor = 5;
        if (!World::TrackData::getTrackMiscData(firstBogie->trackAndDirection.road._data >> 3).sparkDirection)
        {
            unkFactor = -5;
        }

        if (firstBogie->trackAndDirection.road.isReversed())
        {
            unkFactor = -unkFactor;
        }

        xyFactor = Math::Trigonometry::computeXYVector(unkFactor, yaw);
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].objectId);
    }

    // 0x004ABC8A & 0x004AB2A7
    void VehicleBody::shipWakeAnimationUpdate(uint8_t num, int32_t)
    {
        Vehicle2* veh_2 = _vehicleUpdate_2;
        const auto* vehicleObject = getObject();

        if (veh_2->motorState == MotorState::stopped)
        {
            return;
        }

        if (veh_2->currentSpeed < 6.0_mph)
        {
            return;
        }

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

        if ((ScenarioManager::getScenarioTicks() % frequency) != 0)
        {
            return;
        }

        auto positionFactor = vehicleObject->bodySprites[0].halfLength;
        auto invertedDirection = spriteYaw ^ (1 << 5);
        auto xyFactor = Math::Trigonometry::computeXYVector(positionFactor, invertedDirection) / 4;

        World::Pos3 loc = position + World::Pos3(xyFactor.x, xyFactor.y, 0);

        // 90 degrees C.W.
        auto yaw = (spriteYaw + 16) & 0x3F;

        xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->shipWakeOffset, yaw) / 2;
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].objectId);

        if (vehicleObject->shipWakeOffset == 0)
        {
            return;
        }

        // 90 degrees C.C.W.
        yaw = (spriteYaw - 16) & 0x3F;

        xyFactor = Math::Trigonometry::computeXYVector(vehicleObject->shipWakeOffset, yaw) / 2;
        loc.x += xyFactor.x;
        loc.y += xyFactor.y;

        Exhaust::create(loc, vehicleObject->animation[num].objectId);
    }

    // 0x004AC039
    // Note: Vanilla often called this from bogies which would
    // just return immediately
    void VehicleBody::updateCargoSprite()
    {
        if (objectSpriteType == 0xFF)
        {
            return;
        }
        if (primaryCargo.maxQty == 0)
        {
            return;
        }

        const auto* vehicleObj = getObject();
        auto& bodySprite = vehicleObj->bodySprites[objectSpriteType];

        auto percentageFull = std::min((primaryCargo.qty * 256) / primaryCargo.maxQty, 255);
        auto spriteIndex = (percentageFull * bodySprite.numCargoLoadFrames) / 256;
        if (spriteIndex != 0)
        {
            spriteIndex += vehicleObj->cargoTypeSpriteOffsets[primaryCargo.type];
        }
        spriteIndex *= bodySprite.numAnimationFrames;
        if (spriteIndex != cargoFrame)
        {
            cargoFrame = spriteIndex;
            invalidateSprite();
        }
    }

    // 0x0042F6B6
    // TODO: move this?
    uint32_t getNumUnitsForCargo(uint32_t maxPrimaryCargo, uint8_t primaryCargoId, uint8_t newCargoId)
    {
        auto cargoObjA = ObjectManager::get<CargoObject>(primaryCargoId);
        auto cargoObjB = ObjectManager::get<CargoObject>(newCargoId);
        return (cargoObjA->unitSize * maxPrimaryCargo) / cargoObjB->unitSize;
    }
}
