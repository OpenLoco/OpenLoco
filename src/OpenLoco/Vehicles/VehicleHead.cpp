#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Core/Optional.hpp"
#include "../Entities/EntityManager.h"
#include "../Entities/Misc.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Map/TileManager.h"
#include "../MessageManager.h"
#include "../Objects/AirportObject.h"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../StationManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "Orders.h"
#include "Vehicle.h"
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<uint32_t, 0x011360D0> vehicleUpdate_manhattanDistanceToStation;
    static loco_global<VehicleHead*, 0x01136118> vehicleUpdate_head;
    static loco_global<Vehicle1*, 0x0113611C> vehicleUpdate_1;
    static loco_global<Vehicle2*, 0x01136120> vehicleUpdate_2;
    static loco_global<VehicleBogie*, 0x01136124> vehicleUpdate_frontBogie;
    static loco_global<VehicleBogie*, 0x01136128> vehicleUpdate_backBogie;
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<int32_t, 0x01136130> vehicleUpdate_var_1136130; // Speed
    static loco_global<int16_t, 0x01136168> vehicleUpdate_targetZ;
    static loco_global<uint8_t, 0x01136237> vehicle_var_1136237; // var_28 related?
    static loco_global<uint8_t, 0x01136238> vehicle_var_1136238; // var_28 related?
    static loco_global<Status, 0x0113646C> vehicleUpdate_initialStatus;
    static loco_global<uint8_t, 0x0113646D> vehicleUpdate_helicopterTargetYaw;
    static loco_global<uint32_t, 0x00525BB0> vehicleUpdate_var_525BB0;
    static loco_global<int16_t[128], 0x00503B6A> factorXY503B6A;
    static constexpr uint16_t trainOneWaySignalTimeout = 1920;
    static constexpr uint16_t trainTwoWaySignalTimeout = 640;
    static constexpr uint16_t busSignalTimeout = 960;   // Time to wait before turning around at barriers
    static constexpr uint16_t tramSignalTimeout = 2880; // Time to wait before turning around at barriers

    void VehicleHead::updateVehicle()
    {
        // TODO: Refactor to use the Vehicle super class
        VehicleBase* v = this;
        while (v != nullptr)
        {
            if (v->updateComponent())
            {
                break;
            }
            v = v->nextVehicleComponent();
        }
    }

    // 0x004A8B81
    bool VehicleHead::update()
    {
        Vehicle train(this);
        vehicleUpdate_head = train.head;
        vehicleUpdate_1 = train.veh1;
        vehicleUpdate_2 = train.veh2;

        vehicleUpdate_initialStatus = status;
        updateDrivingSounds();

        vehicleUpdate_frontBogie = reinterpret_cast<VehicleBogie*>(0xFFFFFFFF);
        vehicleUpdate_backBogie = reinterpret_cast<VehicleBogie*>(0xFFFFFFFF);

        Vehicle2* veh2 = vehicleUpdate_2;
        vehicleUpdate_var_113612C = veh2->currentSpeed.getRaw() >> 7;
        vehicleUpdate_var_1136130 = veh2->currentSpeed.getRaw() >> 7;

        if (var_5C != 0)
        {
            var_5C--;
        }

        if (tile_x == -1)
        {
            if (!train.cars.empty())
            {
                return false;
            }

            train.tail->trainDanglingTimeout++;
            if (train.tail->trainDanglingTimeout < 960)
            {
                return false;
            }
            removeDanglingTrain();
            return false;
        }
        updateBreakdown();
        bool continueUpdating = true;
        switch (mode)
        {
            case TransportMode::rail:
            case TransportMode::road:
                continueUpdating = updateLand();
                break;
            case TransportMode::air:
                continueUpdating = updateAir();
                if (continueUpdating)
                {
                    tryCreateInitialMovementSound();
                }
                break;
            case TransportMode::water:
                continueUpdating = updateWater();
                if (continueUpdating)
                {
                    tryCreateInitialMovementSound();
                }
                break;
        }
        // TODO move to here when all update mode functions implemented
        //if (continueUpdating)
        //{
        //    tryCreateInitialMovementSound();
        //}
        return continueUpdating;
    }

    // 0x004BA8D4
    void VehicleHead::updateBreakdown()
    {
        switch (status)
        {
            case Status::unk_0:
            case Status::stopped:
            case Status::unk_3:
            case Status::unloading:
            case Status::loading:
            case Status::crashed:
            case Status::stuck:
                return;
            case Status::unk_2:
            case Status::approaching:
            case Status::brokenDown:
            case Status::landing:
            case Status::taxiing1:
            case Status::taxiing2:
            case Status::takingOff:
                break;
        }
        Vehicle train(this);
        for (auto& car : train.cars)
        {
            if (car.front->var_5F & Flags5F::broken_down)
            {
                if ((scenarioTicks() & 3) == 0)
                {
                    auto v2 = car.body; // body
                    Smoke::create(Map::map_pos3(v2->x, v2->y, v2->z + 4));
                }
            }

            if ((car.front->var_5F & Flags5F::breakdown_pending) && !isTitleMode())
            {
                auto newConfig = Config::getNew();
                if (!newConfig.breakdowns_disabled)
                {
                    car.front->var_5F &= ~Flags5F::breakdown_pending;
                    car.front->var_5F |= Flags5F::broken_down;
                    car.front->var_6A = 5;
                    applyBreakdownToTrain();

                    auto v2 = car.body;
                    auto soundId = (Audio::sound_id)gPrng().randNext(26, 26 + 5);
                    Audio::playSound(soundId, Map::map_pos3(v2->x, v2->y, v2->z + 22));
                }
            }
        }
    }

    // 0x004BAA76
    void VehicleHead::applyBreakdownToTrain()
    {
        Vehicle train(this);
        bool isBrokenDown = false;
        bool trainStillPowered = false;
        // Check only the first bogie on each car for breakdown flags
        for (const auto& car : train.cars)
        {
            auto* vehObj = ObjectManager::get<VehicleObject>(car.front->object_id);
            if (vehObj == nullptr)
            {
                continue;
            }
            // Unpowered vehicles can not breakdown
            if (vehObj->power == 0)
            {
                continue;
            }
            if (car.front->var_5F & Flags5F::broken_down)
            {
                isBrokenDown = true;
            }
            else
            {
                trainStillPowered = true;
            }
        }
        if (isBrokenDown)
        {
            if (trainStillPowered)
            {
                train.veh2->var_73 |= Flags73::isBrokenDown | Flags73::isStillPowered;
            }
            else
            {
                train.veh2->var_73 |= Flags73::isBrokenDown;
                train.veh2->var_73 &= ~Flags73::isStillPowered;
            }
        }
        else
        {
            train.veh2->var_73 &= ~(Flags73::isBrokenDown | Flags73::isStillPowered);
        }
    }

    // 0x004B90F0
    // eax : newVehicleTypeId
    // ebx : sourceVehicleTypeId;
    static bool sub_4B90F0(const uint16_t newVehicleTypeId, const uint16_t sourceVehicleTypeId)
    {
        auto newObject = ObjectManager::get<VehicleObject>(newVehicleTypeId);       //edi
        auto sourceObject = ObjectManager::get<VehicleObject>(sourceVehicleTypeId); // esi

        if ((newObject->flags & FlagsE0::can_couple) && (sourceObject->flags & FlagsE0::can_couple))
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
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
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        return true;
    }

    // 0x004B9780
    // used by road vehicles only maybe??
    static uint32_t getVehicleTypeLength(const uint16_t vehicleTypeId)
    {
        auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);
        auto length = 0;
        for (auto i = 0; i < vehObject->var_04; ++i)
        {
            if (vehObject->var_24[i].body_sprite_ind == 0xFF)
            {
                continue;
            }

            auto unk = vehObject->var_24[i].body_sprite_ind & 0x7F;
            length += vehObject->bodySprites[unk].bogey_position * 2;
        }
        return length;
    }

    // 0x004B97B7
    // used by road vehicles only maybe??
    uint32_t VehicleHead::getVehicleTotalLength() // TODO: const
    {
        auto totalLength = 0;
        Vehicle train(this);
        for (const auto& car : train.cars)
        {
            totalLength += getVehicleTypeLength(car.body->object_id);
        }
        return totalLength;
    }

    // 0x004B8FA2
    // esi : self
    // ax  : vehicleTypeId
    bool VehicleHead::isVehicleTypeCompatible(const uint16_t vehicleTypeId) // TODO: const
    {
        auto newObject = ObjectManager::get<VehicleObject>(vehicleTypeId);
        if (newObject->mode == TransportMode::air || newObject->mode == TransportMode::water)
        {
            Vehicle train(this);
            if (!train.cars.empty())
            {
                gGameCommandErrorText = StringIds::incompatible_vehicle;
                return false;
            }
        }
        else
        {
            if (newObject->track_type != track_type)
            {
                gGameCommandErrorText = StringIds::incompatible_vehicle;
                return false;
            }
        }

        if (newObject->mode != mode)
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        if (newObject->type != vehicleType)
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        {
            Vehicle train(this);
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
        if (curTotalLength + additionalNewLength > max_vehicle_length)
        {
            gGameCommandErrorText = StringIds::vehicle_too_long;
            return false;
        }
        return true;
    }

    // 0x004B671C
    VehicleStatus VehicleHead::getStatus() const
    {
        registers regs = {};
        regs.esi = reinterpret_cast<int32_t>(this);

        call(0x004B671C, regs);

        VehicleStatus vehStatus = {};
        vehStatus.status1 = regs.bx;
        vehStatus.status1Args = regs.eax;
        vehStatus.status2 = regs.cx;
        vehStatus.status2Args = regs.edx;
        return vehStatus;
    }

    // 0x004A8882
    void VehicleHead::updateDrivingSounds()
    {
        Vehicle train(this);
        updateDrivingSound(train.veh2->asVehicle2Or6());
        updateDrivingSound(train.tail->asVehicle2Or6());
    }

    // 0x004A88A6
    void VehicleHead::updateDrivingSound(Vehicle2or6* vehType2or6)
    {
        if (tile_x == -1 || status == Status::crashed || status == Status::stuck || (var_38 & Flags38::isGhost) || vehType2or6->objectId == 0xFFFF)
        {
            updateDrivingSoundNone(vehType2or6);
            return;
        }

        auto vehicleObject = ObjectManager::get<VehicleObject>(vehType2or6->objectId);
        switch (vehicleObject->drivingSoundType)
        {
            case DrivingSoundType::none:
                updateDrivingSoundNone(vehType2or6);
                break;
            case DrivingSoundType::friction:
                updateDrivingSoundFriction(vehType2or6, &vehicleObject->sound.friction);
                break;
            case DrivingSoundType::engine1:
                updateDrivingSoundEngine1(vehType2or6, &vehicleObject->sound.engine1);
                break;
            case DrivingSoundType::engine2:
                updateDrivingSoundEngine2(vehType2or6, &vehicleObject->sound.engine2);
                break;
            default:
                break;
        }
    }

    // 0x004A8B7C
    void VehicleHead::updateDrivingSoundNone(Vehicle2or6* vehType2or6)
    {
        vehType2or6->drivingSoundId = 0xFF;
    }

    // 0x004A88F7
    void VehicleHead::updateDrivingSoundFriction(Vehicle2or6* vehType2or6, VehicleObjectFrictionSound* snd)
    {
        Vehicle2* vehType2_2 = vehicleUpdate_2;
        if (vehType2_2->currentSpeed < snd->minSpeed)
        {
            updateDrivingSoundNone(vehType2or6);
            return;
        }

        auto speedDiff = vehType2_2->currentSpeed - snd->minSpeed;
        vehType2or6->drivingSoundFrequency = (speedDiff.getRaw() >> snd->speedFreqFactor) + snd->baseFrequency;

        auto volume = (speedDiff.getRaw() >> snd->speedVolumeFactor) + snd->baseVolume;

        vehType2or6->drivingSoundVolume = std::min<uint8_t>(volume, snd->maxVolume);
        vehType2or6->drivingSoundId = snd->soundObjectId;
    }

    // 0x004A8937
    void VehicleHead::updateDrivingSoundEngine1(Vehicle2or6* vehType2or6, VehicleObjectEngine1Sound* snd)
    {
        Vehicle train(this);
        if (vehType2or6->isVehicle2())
        {
            if (vehicleType != VehicleType::ship && vehicleType != VehicleType::aircraft)
            {
                // Can be a type 6 or bogie
                if (train.cars.empty())
                {
                    assert(false);
                }
                if (train.cars.firstCar.front->var_5F & Flags5F::broken_down)
                {
                    updateDrivingSoundNone(vehType2or6);
                    return;
                }
            }
        }

        Vehicle2* vehType2_2 = vehicleUpdate_2;
        uint16_t targetFrequency = 0;
        uint8_t targetVolume = 0;
        if (vehType2_2->var_5A == 2)
        {
            if (vehType2_2->currentSpeed < 12.0_mph)
            {
                targetFrequency = snd->defaultFrequency;
                targetVolume = snd->defaultVolume;
            }
            else
            {
                targetFrequency = snd->var_04;
                targetVolume = snd->var_06;
            }
        }
        else if (vehType2_2->var_5A == 1)
        {
            if (!(vehType2or6->isVehicle2()) || train.cars.firstCar.front->var_5E == 0)
            {
                targetFrequency = snd->var_07 + (vehType2_2->currentSpeed.getRaw() >> snd->speedFreqFactor);
                targetVolume = snd->var_09;
            }
            else
            {
                targetFrequency = snd->defaultFrequency;
                targetVolume = snd->defaultVolume;
            }
        }
        else
        {
            targetFrequency = snd->defaultFrequency;
            targetVolume = snd->defaultVolume;
        }

        if (vehType2or6->drivingSoundId == 0xFF)
        {
            // Half
            vehType2or6->drivingSoundVolume = snd->defaultVolume >> 1;
            // Quarter
            vehType2or6->drivingSoundFrequency = snd->defaultFrequency >> 2;
            vehType2or6->drivingSoundId = snd->soundObjectId;
            return;
        }

        if (vehType2or6->drivingSoundFrequency != targetFrequency)
        {
            if (vehType2or6->drivingSoundFrequency > targetFrequency)
            {
                vehType2or6->drivingSoundFrequency = std::max<uint16_t>(targetFrequency, vehType2or6->drivingSoundFrequency - snd->freqDecreaseStep);
            }
            else
            {
                vehType2or6->drivingSoundFrequency = std::min<uint16_t>(targetFrequency, vehType2or6->drivingSoundFrequency + snd->freqIncreaseStep);
            }
        }

        if (vehType2or6->drivingSoundVolume != targetVolume)
        {
            if (vehType2or6->drivingSoundVolume > targetVolume)
            {
                vehType2or6->drivingSoundVolume = std::max<uint8_t>(targetVolume, vehType2or6->drivingSoundVolume - snd->volumeDecreaseStep);
            }
            else
            {
                vehType2or6->drivingSoundVolume = std::min<uint8_t>(targetVolume, vehType2or6->drivingSoundVolume + snd->volumeIncreaseStep);
            }
        }

        vehType2or6->drivingSoundId = snd->soundObjectId;
    }

    // 0x004A8A39
    void VehicleHead::updateDrivingSoundEngine2(Vehicle2or6* vehType2or6, VehicleObjectEngine2Sound* snd)
    {
        Vehicle train(this);
        if (vehType2or6->isVehicle2())
        {
            if (vehicleType != VehicleType::ship && vehicleType != VehicleType::aircraft)
            {
                // Can be a type 6 or bogie
                if (train.cars.empty())
                {
                    assert(false);
                }
                if (train.cars.firstCar.front->var_5F & Flags5F::broken_down)
                {
                    updateDrivingSoundNone(vehType2or6);
                    return;
                }
            }
        }

        Vehicle2* vehType2_2 = vehicleUpdate_2;
        uint16_t targetFrequency = 0;
        uint8_t targetVolume = 0;
        bool var5aEqual1Code = false;

        if (vehType2_2->var_5A == 2 || vehType2_2->var_5A == 3)
        {
            if (vehType2_2->currentSpeed < 12.0_mph)
            {
                targetFrequency = snd->defaultFrequency;
                targetVolume = snd->var_12;
            }
            else
            {
                targetVolume = snd->var_12;
                var5aEqual1Code = true;
            }
        }
        else if (vehType2_2->var_5A == 1)
        {
            targetVolume = snd->var_13;
            var5aEqual1Code = true;
        }
        else
        {
            targetFrequency = snd->defaultFrequency;
            targetVolume = snd->defaultVolume;
        }

        if (var5aEqual1Code == true)
        {
            if (!(vehType2or6->isVehicle2()) || train.cars.firstCar.front->var_5E == 0)
            {
                auto speed = std::max(vehType2_2->currentSpeed, 7.0_mph);

                auto frequency = snd->firstGearFrequency;

                if (speed >= snd->firstGearSpeed)
                {
                    frequency -= snd->secondGearFreqFactor;
                    if (speed >= snd->secondGearSpeed)
                    {
                        frequency -= snd->thirdGearFreqFactor;
                        if (speed >= snd->thirdGearSpeed)
                        {
                            frequency -= snd->fourthGearFreqFactor;
                        }
                    }
                }
                targetFrequency = (speed.getRaw() >> snd->speedFreqFactor) + frequency;
            }
            else
            {
                targetFrequency = snd->defaultFrequency;
                targetVolume = snd->defaultVolume;
            }
        }

        if (vehType2or6->drivingSoundId == 0xFF)
        {
            // Half
            vehType2or6->drivingSoundVolume = snd->defaultVolume >> 1;
            // Quarter
            vehType2or6->drivingSoundFrequency = snd->defaultFrequency >> 2;
            vehType2or6->drivingSoundId = snd->soundObjectId;
            return;
        }

        if (vehType2or6->drivingSoundFrequency != targetFrequency)
        {
            if (vehType2or6->drivingSoundFrequency > targetFrequency)
            {
                targetVolume = snd->var_12;
                vehType2or6->drivingSoundFrequency = std::max<uint16_t>(targetFrequency, vehType2or6->drivingSoundFrequency - snd->freqDecreaseStep);
            }
            else
            {
                vehType2or6->drivingSoundFrequency = std::min<uint16_t>(targetFrequency, vehType2or6->drivingSoundFrequency + snd->freqIncreaseStep);
            }
        }

        if (vehType2or6->drivingSoundVolume != targetVolume)
        {
            if (vehType2or6->drivingSoundVolume > targetVolume)
            {
                vehType2or6->drivingSoundVolume = std::max<uint8_t>(targetVolume, vehType2or6->drivingSoundVolume - snd->volumeDecreaseStep);
            }
            else
            {
                vehType2or6->drivingSoundVolume = std::min<uint8_t>(targetVolume, vehType2or6->drivingSoundVolume + snd->volumeIncreaseStep);
            }
        }

        vehType2or6->drivingSoundId = snd->soundObjectId;
    }

    // 0x004AF06E
    void VehicleHead::removeDanglingTrain()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004AF06E, regs);
    }

    // Returns veh1, veh2 position
    static std::pair<map_pos, map_pos> calculateNextPosition(const uint8_t yaw, const Map::map_pos& curPos, const Vehicle1* veh1, const Speed32 speed)
    {
        auto distX = (speed.getRaw() >> 13) * factorXY503B6A[yaw * 2];
        auto distY = (speed.getRaw() >> 13) * factorXY503B6A[yaw * 2 + 1];

        auto bigCoordX = veh1->var_4E + (curPos.x << 16) + distX;
        auto bigCoordY = veh1->var_50 + (curPos.y << 16) + distY;

        map_pos veh1Pos = { static_cast<int16_t>(bigCoordX & 0xFFFF), static_cast<int16_t>(bigCoordY & 0xFFFF) };
        map_pos veh2Pos = { static_cast<int16_t>(bigCoordX >> 16), static_cast<int16_t>(bigCoordY >> 16) };
        return std::make_pair(veh1Pos, veh2Pos);
    }

    // 0x004A8C11
    bool VehicleHead::updateLand()
    {
        Vehicle2* vehType2 = vehicleUpdate_2;
        if ((!(vehType2->var_73 & Flags73::isBrokenDown) || (vehType2->var_73 & Flags73::isStillPowered)) && status == Status::approaching)
        {
            if (mode == TransportMode::road)
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

            if (var_0C & Flags0C::commandStop)
            {
                return sub_4A8CB6();
            }
            else if (var_0C & Flags0C::manualControl)
            {
                if (var_6E <= -20)
                {
                    return sub_4A8C81();
                }
            }

            return landTryBeginUnloading();
        }

        if (status == Status::unloading)
        {
            updateUnloadCargo();

            tryCreateInitialMovementSound();
            return true;
        }
        else if (status == Status::loading)
        {
            return landLoadingUpdate();
        }
        else if (status == Status::crashed)
        {
            sub_4AA625();

            return false;
        }
        else if (status == Status::stuck)
        {
            return false;
        }
        else
        {
            status = Status::unk_2;

            if (!(vehType2->var_73 & Flags73::isBrokenDown) || (vehType2->var_73 & Flags73::isStillPowered))
            {
                if (!(var_0C & Flags0C::manualControl) || var_6E > -20)
                {
                    if (!(var_0C & Flags0C::commandStop))
                    {
                        return landNormalMovementUpdate();
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

    // 0x004AA36A
    // 0: None of the below
    // 1: reached first timeout at signal
    // 2: give up and reverse at signal
    uint8_t VehicleHead::sub_4AA36A()
    {
        Vehicle train(this);
        if (train.veh2->var_36 != train.veh1->var_36 || train.veh2->var_2E != train.veh1->var_2E)
        {
            train.veh1->timeAtSignal = 0;
            return 0;
        }

        auto param1 = 160;
        auto turnaroundAtSignalTimeout = busSignalTimeout;

        if (track_type == 0xFF || ObjectManager::get<RoadObject>(track_type)->flags & Flags12::isRoad)
        {
            if (train.veh1->var_2C & (1 << 7))
            {
                param1 = 128;
                turnaroundAtSignalTimeout = 544;
            }
        }
        else
        {
            // Tram
            turnaroundAtSignalTimeout = tramSignalTimeout;
            if (train.veh1->var_2C & (1 << 7))
            {
                param1 = 64;
                turnaroundAtSignalTimeout = 128;
            }
        }

        train.veh1->timeAtSignal++;
        if (train.veh1->timeAtSignal == param1)
        {
            return 1;
        }

        if (train.veh1->timeAtSignal == turnaroundAtSignalTimeout)
        {
            var_5C = 40;
            return 2;
        }

        return 0;
    }

    // 0x004A8DB7
    bool VehicleHead::sub_4A8DB7()
    {
        sub_4AD778();
        if (status == Status::approaching)
        {
            status = Status::unk_2;
        }
        tryCreateInitialMovementSound();
        return true;
    }

    // 0x004A8F22
    bool VehicleHead::sub_4A8F22()
    {
        if (isOnExpectedRoadOrTrack())
        {
            auto temp = var_52;
            var_52 = 1;
            sub_4ADB47(false);
            var_52 = temp;
            tryCreateInitialMovementSound();
            return true;
        }
        else
        {
            Vehicle train(this);
            train.veh2->sub_4AA464();
            return false;
        }
    }

    // 0x004A8CB6
    bool VehicleHead::sub_4A8CB6()
    {
        Vehicle1* vehType1 = vehicleUpdate_1;

        if (x != vehType1->x || y != vehType1->y || z != vehType1->z)
        {
            sub_4AD93A();
            if (status == Status::approaching)
            {
                stationId = StationId::null;
                status = Status::unk_2;
            }
        }

        Vehicle train(this);
        auto* vehType2 = train.veh2;
        if (vehType2->var_36 != var_36 || vehType2->var_2E != var_2E)
        {
            tryCreateInitialMovementSound();
            return true;
        }

        status = Status::stopped;
        vehType2 = vehicleUpdate_2;

        if (vehType2->var_73 & Flags73::isBrokenDown)
        {
            stationId = StationId::null;
            status = Status::brokenDown;

            tryCreateInitialMovementSound();
            return true;
        }
        tryCreateInitialMovementSound();
        return true;
    }

    // 0x004A8C81
    bool VehicleHead::sub_4A8C81()
    {
        Vehicle2* vehType2 = vehicleUpdate_2;
        if (vehType2->currentSpeed > 1.0_mph)
        {
            return landNormalMovementUpdate();
        }

        auto foundStationId = manualFindTrainStationAtLocation();
        if (foundStationId == StationId::null)
        {
            return sub_4A8CB6();
        }
        stationId = foundStationId;
        setStationVisitedTypes();
        checkIfAtOrderStation();
        updateLastJourneyAverageSpeed();
        beginUnloading();

        return sub_4A8CB6();
    }

    // 0x004A8FAC
    // Checks if at the desiered station and then begins unloading if at it
    bool VehicleHead::landTryBeginUnloading()
    {
        Vehicle train(this);
        if (var_36 != train.veh2->var_36 || train.veh2->var_2E != var_2E)
        {
            tryCreateInitialMovementSound();
            return true;
        }

        // Manual control is going too fast at this point to stop at the station
        if (var_0C & Flags0C::manualControl)
        {
            tryCreateInitialMovementSound();
            return true;
        }

        setStationVisitedTypes();
        checkIfAtOrderStation();
        updateLastJourneyAverageSpeed();
        beginUnloading();

        tryCreateInitialMovementSound();
        return true;
    }

    // 0x004A9011
    bool VehicleHead::landLoadingUpdate()
    {
        if (updateLoadCargo())
        {
            tryCreateInitialMovementSound();
            return true;
        }

        beginNewJourney();
        status = Status::stopped;
        advanceToNextRoutableOrder();

        if (var_0C & Flags0C::manualControl)
        {
            tryCreateInitialMovementSound();
            return true;
        }

        if (sub_4ACCDC())
        {
            return sub_4A8F22();
        }

        tryCreateInitialMovementSound();
        return true;
    }

    // 0x004A8D48
    bool VehicleHead::landNormalMovementUpdate()
    {
        advanceToNextRoutableOrder();
        auto [al, flags, nextStation] = sub_4ACEE7(0xD4CB00, vehicleUpdate_var_113612C);

        if (mode == TransportMode::road)
        {
            return roadNormalMovementUpdate(al, nextStation);
        }
        else
        {
            return trainNormalMovementUpdate(al, flags, nextStation);
        }
    }

    // 0x004A8D8F
    bool VehicleHead::roadNormalMovementUpdate(uint8_t al, station_id_t nextStation)
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
            status = Status::approaching;
            stationId = nextStation;
            tryCreateInitialMovementSound();
            return true;
        }
        else if (al == 2)
        {
            Vehicle train(this);
            if (var_36 != train.veh2->var_36 || train.veh2->var_2E != var_2E)
            {
                tryCreateInitialMovementSound();
                return true;
            }
            return sub_4A8F22();
        }
        else
        {
            tryCreateInitialMovementSound();
            return true;
        }
    }

    // 0x004A8D63
    bool VehicleHead::trainNormalMovementUpdate(uint8_t al, uint8_t flags, station_id_t nextStation)
    {
        Vehicle train(this);
        if (al == 4)
        {
            status = Status::approaching;
            stationId = nextStation;
            tryCreateInitialMovementSound();
            return true;
        }
        else if (al == 3)
        {
            if (train.veh2->var_36 != var_36 || train.veh2->var_2E != var_2E)
            {
                tryCreateInitialMovementSound();
                return true;
            }

            status = Status::unk_3;

            auto* vehType1 = train.veh1;
            vehType1->timeAtSignal++;

            if (var_0C & Flags0C::manualControl)
            {
                var_5C = 2;
                vehType1->var_48 |= 1 << 0;
                tryCreateInitialMovementSound();
                return true;
            }

            // if one-way or two-way signal??
            if (flags & (1 << 1))
            {
                if (vehType1->timeAtSignal >= trainOneWaySignalTimeout)
                {
                    if (flags & (1 << 7))
                    {
                        return landReverseFromSignal();
                    }

                    if (sub_4AC1C2())
                    {
                        var_5C = 2;
                        vehType1->var_48 |= 1 << 0;
                        tryCreateInitialMovementSound();
                        return true;
                    }
                    return landReverseFromSignal();
                }

                // Keep waiting at the signal
                tryCreateInitialMovementSound();
                return true;
            }
            else
            {
                if (!(vehType1->timeAtSignal & 0x3F))
                {
                    if (!(flags & (1 << 7)))
                    {
                        if (sub_4AC1C2())
                        {
                            var_5C = 2;
                            vehType1->var_48 |= 1 << 0;
                            tryCreateInitialMovementSound();
                            return true;
                        }
                    }

                    if (sub_4AC0A3())
                    {
                        return landReverseFromSignal();
                    }
                }

                if (vehType1->timeAtSignal >= trainTwoWaySignalTimeout)
                {
                    return landReverseFromSignal();
                }

                // Keep waiting at the signal
                tryCreateInitialMovementSound();
                return true;
            }
        }
        else
        {
            train.veh1->timeAtSignal = 0;
            if (al == 2)
            {
                if (var_0C & Flags0C::manualControl)
                {
                    auto* vehType2 = train.veh2;
                    if (vehType2->var_36 != var_36 || vehType2->var_2E != var_2E)
                    {
                        return landReverseFromSignal();
                    }

                    // Crash
                    vehType2->sub_4AA464();
                    return false;
                }

                return landReverseFromSignal();
            }
            else
            {
                tryCreateInitialMovementSound();
                return true;
            }
        }
    }

    // 0x004A8ED9
    bool VehicleHead::landReverseFromSignal()
    {
        Vehicle train(this);
        train.veh1->timeAtSignal = 0;

        if (var_36 != train.veh2->var_36 || train.veh2->var_2E != var_2E)
        {
            tryCreateInitialMovementSound();
            return true;
        }
        return sub_4A8F22();
    }

    // 0x004A9051
    bool VehicleHead::updateAir()
    {
        Vehicle2* vehType2 = vehicleUpdate_2;

        if (vehType2->currentSpeed >= 20.0_mph)
        {
            vehicleUpdate_var_1136130 = 0x4000;
        }
        else
        {
            vehicleUpdate_var_1136130 = 0x2000;
        }

        Vehicle train(this);
        train.cars.firstCar.body->sub_4AAB0B();

        if (status == Status::stopped)
        {
            if (!(var_0C & Flags0C::commandStop))
            {
                setStationVisitedTypes();
                checkIfAtOrderStation();
                beginUnloading();
            }
            return true;
        }
        else if (status == Status::unloading)
        {
            updateUnloadCargo();
            return true;
        }
        else if (status == Status::loading)
        {
            return airplaneLoadingUpdate();
        }
        status = Status::unk_2;
        auto [newStatus, targetSpeed] = airplaneGetNewStatus();

        status = newStatus;
        Vehicle1* vehType1 = vehicleUpdate_1;
        vehType1->var_44 = targetSpeed;

        advanceToNextRoutableOrder();
        Speed32 type1speed = vehType1->var_44;
        auto type2speed = vehType2->currentSpeed;

        if (type2speed == type1speed)
        {
            vehType2->var_5A = 5;

            if (type2speed != 20.0_mph)
            {
                vehType2->var_5A = 2;
            }
        }
        else if (type2speed > type1speed)
        {
            vehType2->var_5A = 2;
            auto decelerationAmount = 2.0_mph;
            if (type2speed >= 130.0_mph)
            {
                decelerationAmount = 5.0_mph;
                if (type2speed >= 400.0_mph)
                {
                    decelerationAmount = 11.0_mph;
                    if (type2speed >= 600.0_mph)
                    {
                        decelerationAmount = 25.0_mph;
                    }
                }
            }

            if (type1speed == 20.0_mph)
            {
                vehType2->var_5A = 3;
            }

            type2speed = std::max<Speed32>(0.0_mph, type2speed - decelerationAmount);
            type2speed = std::max<Speed32>(type1speed, type2speed);
            vehType2->currentSpeed = type2speed;
        }
        else
        {
            vehType2->var_5A = 1;
            type2speed += 2.0_mph;
            type2speed = std::min<Speed32>(type2speed, type1speed);
            vehType2->currentSpeed = type2speed;
        }

        auto [manhattanDistance, targetZ, targetYaw] = sub_427122();

        vehicleUpdate_manhattanDistanceToStation = manhattanDistance;
        vehicleUpdate_targetZ = targetZ;

        // Helicopter
        if (vehicleUpdate_var_525BB0 & AirportMovementNodeFlags::heliTakeoffEnd)
        {
            vehicleUpdate_helicopterTargetYaw = targetYaw;
            targetYaw = sprite_yaw;
            vehType2->var_5A = 1;
            if (targetZ < z)
            {
                vehType2->var_5A = 2;
            }
        }

        if (targetYaw != sprite_yaw)
        {
            if (((targetYaw - sprite_yaw) & 0x3F) > 0x20)
            {
                sprite_yaw--;
            }
            else
            {
                sprite_yaw++;
            }
            sprite_yaw &= 0x3F;
        }

        Pitch targetPitch = Pitch::flat;
        if (vehType2->currentSpeed < 50.0_mph)
        {
            auto vehObject = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->object_id);
            targetPitch = Pitch::up12deg;
            // Slope sprites for taxiing planes??
            if (!(vehObject->flags & FlagsE0::unk_08))
            {
                targetPitch = Pitch::flat;
            }
        }

        if (targetZ > z)
        {
            if (vehType2->currentSpeed <= 350.0_mph)
            {
                targetPitch = Pitch::up12deg;
            }
        }

        if (targetZ < z)
        {
            if (vehType2->currentSpeed <= 180.0_mph)
            {
                auto vehObject = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->object_id);

                // looks wrong??
                if (vehObject->flags & FlagsE0::can_couple)
                {
                    targetPitch = Pitch::up12deg;
                }
            }
        }

        if (targetPitch != sprite_pitch)
        {
            if (targetPitch < sprite_pitch)
            {
                sprite_pitch = Pitch(static_cast<uint8_t>(sprite_pitch) - 1);
            }
            else
            {
                sprite_pitch = Pitch(static_cast<uint8_t>(sprite_pitch) + 1);
            }
        }

        // Helicopter
        if (vehicleUpdate_var_525BB0 & AirportMovementNodeFlags::heliTakeoffEnd)
        {
            vehType2->currentSpeed = 8.0_mph;
            if (targetZ != z)
            {
                return airplaneApproachTarget(targetZ);
            }
        }
        else
        {
            uint32_t targetTolerance = 480;
            if (airportMovementEdge != cAirportMovementNodeNull)
            {
                targetTolerance = 5;
                if (vehType2->currentSpeed >= 70.0_mph)
                {
                    targetTolerance = 30;
                }
            }

            if (manhattanDistance > targetTolerance)
            {
                return airplaneApproachTarget(targetZ);
            }
        }

        if (stationId != StationId::null && airportMovementEdge != cAirportMovementNodeNull)
        {
            auto flags = airportGetMovementEdgeTarget(stationId, airportMovementEdge).first;

            if (flags & AirportMovementNodeFlags::touchdown)
            {
                produceTouchdownAirportSound();
            }
            if (flags & AirportMovementNodeFlags::taxiing)
            {
                updateLastJourneyAverageSpeed();
            }

            if (flags & AirportMovementNodeFlags::terminal)
            {
                return sub_4A95CB();
            }
        }

        auto movementEdge = cAirportMovementNodeNull;
        if (stationId != StationId::null)
        {
            movementEdge = airportMovementEdge;
        }

        auto newMovementEdge = airportGetNextMovementEdge(movementEdge);

        if (newMovementEdge != static_cast<uint8_t>(-2))
        {
            return sub_4A9348(newMovementEdge, targetZ);
        }

        if (vehType2->currentSpeed > 30.0_mph)
        {
            return airplaneApproachTarget(targetZ);
        }
        else
        {
            vehType2->currentSpeed = 0.0_mph;
            vehType2->var_5A = 0;
            return true;
        }
    }

    // 0x004273DF
    std::pair<Status, Speed16> VehicleHead::airplaneGetNewStatus()
    {
        Vehicle train(this);
        if (stationId == StationId::null || airportMovementEdge == cAirportMovementNodeNull)
        {
            auto veh2 = train.veh2;
            auto targetSpeed = veh2->maxSpeed;
            if (veh2->var_73 & Flags73::isBrokenDown)
            {
                targetSpeed = veh2->rackRailMaxSpeed;
            }

            return std::make_pair(Status::unk_2, targetSpeed);
        }

        auto station = StationManager::get(stationId);

        map_pos3 loc = {
            station->unk_tile_x,
            station->unk_tile_y,
            station->unk_tile_z
        };

        auto tile = Map::TileManager::get(loc);
        for (auto& el : tile)
        {
            auto elStation = el.asStation();
            if (elStation == nullptr)
                continue;

            if (elStation->baseZ() != loc.z / 4)
                continue;

            auto airportObject = ObjectManager::get<AirportObject>(elStation->objectId());

            uint8_t al = airportObject->movementEdges[airportMovementEdge].var_03;
            uint8_t cl = airportObject->movementEdges[airportMovementEdge].var_00;

            auto veh2 = train.veh2;
            if (al != 0)
            {
                if (cl == 1 || al != 2)
                {
                    if (al == 1)
                    {
                        return std::make_pair(Status::landing, veh2->rackRailMaxSpeed);
                    }
                    else if (al == 3)
                    {
                        return std::make_pair(Status::landing, 0_mph);
                    }
                    else if (al == 4)
                    {
                        return std::make_pair(Status::taxiing1, 20_mph);
                    }
                    else
                    {
                        return std::make_pair(Status::approaching, veh2->rackRailMaxSpeed);
                    }
                }
            }

            if (cl == 2)
            {
                auto targetSpeed = veh2->maxSpeed;
                if (veh2->var_73 & Flags73::isBrokenDown)
                {
                    targetSpeed = veh2->rackRailMaxSpeed;
                }
                return std::make_pair(Status::takingOff, targetSpeed);
            }
            else if (cl == 3)
            {
                return std::make_pair(Status::takingOff, 0_mph);
            }
            else
            {
                return std::make_pair(Status::taxiing2, 20_mph);
            }
        }

        // Tile not found. Todo: fail gracefully
        assert(false);
        return std::make_pair(Status::unk_2, train.veh2->maxSpeed);
    }

    // 0x004A95CB
    bool VehicleHead::sub_4A95CB()
    {
        if (var_0C & Flags0C::commandStop)
        {
            status = Status::stopped;
            Vehicle2* vehType2 = vehicleUpdate_2;
            vehType2->currentSpeed = 0.0_mph;
        }
        else
        {
            setStationVisitedTypes();
            checkIfAtOrderStation();
            beginUnloading();
        }
        return true;
    }

    // 0x004A95F5
    bool VehicleHead::airplaneLoadingUpdate()
    {
        Vehicle2* vehType2 = vehicleUpdate_2;
        vehType2->currentSpeed = 0.0_mph;
        vehType2->var_5A = 0;
        if (updateLoadCargo())
        {
            return true;
        }

        advanceToNextRoutableOrder();
        status = Status::unk_2;
        status = airplaneGetNewStatus().first;

        auto movementEdge = cAirportMovementNodeNull;
        if (stationId != StationId::null)
        {
            movementEdge = airportMovementEdge;
        }

        auto newMovementEdge = airportGetNextMovementEdge(movementEdge);

        if (newMovementEdge != static_cast<uint8_t>(-2))
        {
            // Strangely the original would enter this function with an
            // uninitialised targetZ. We will pass a valid z.
            return sub_4A9348(newMovementEdge, z);
        }

        status = Status::loading;
        return true;
    }

    // 0x004A94A9
    bool VehicleHead::airplaneApproachTarget(uint16_t targetZ)
    {
        auto _yaw = sprite_yaw;
        // Helicopter
        if (vehicleUpdate_var_525BB0 & AirportMovementNodeFlags::heliTakeoffEnd)
        {
            _yaw = vehicleUpdate_helicopterTargetYaw;
        }

        Vehicle1* vehType1 = vehicleUpdate_1;
        Vehicle2* vehType2 = vehicleUpdate_2;

        auto [veh1Loc, veh2Loc] = calculateNextPosition(
            _yaw, { x, y }, vehType1, vehType2->currentSpeed);

        map_pos3 newLoc(veh2Loc.x, veh2Loc.y, targetZ);
        vehType1->var_4E = veh1Loc.x;
        vehType1->var_50 = veh1Loc.y;
        if (targetZ != z)
        {
            // Final section of landing / helicopter
            if (vehicleUpdate_manhattanDistanceToStation <= 28)
            {
                int16_t z_shift = 1;
                if (vehType2->currentSpeed >= 50.0_mph)
                {
                    z_shift++;
                    if (vehType2->currentSpeed >= 100.0_mph)
                    {
                        z_shift++;
                    }
                }

                if (targetZ < z)
                {
                    newLoc.z = std::max<int16_t>(targetZ, z - z_shift);
                }
                else if (targetZ > z)
                {
                    newLoc.z = std::min<int16_t>(targetZ, z + z_shift);
                }
            }
            else
            {
                int32_t zDiff = targetZ - z;
                // We want a SAR instruction so use >>5
                int32_t param1 = (zDiff * toSpeed16(vehType2->currentSpeed).getRaw()) >> 5;
                int32_t param2 = vehicleUpdate_manhattanDistanceToStation - 18;

                auto modulo = param1 % param2;
                if (modulo < 0)
                {
                    newLoc.z = z + param1 / param2 - 1;
                }
                else
                {
                    newLoc.z = z + param1 / param2 + 1;
                }
            }
        }
        movePlaneTo(newLoc, sprite_yaw, sprite_pitch);
        return true;
    }

    bool VehicleHead::sub_4A9348(uint8_t newMovementEdge, uint16_t targetZ)
    {
        if (stationId != StationId::null && airportMovementEdge != cAirportMovementNodeNull)
        {
            StationManager::get(stationId)->airportMovementOccupiedEdges &= ~(1 << airportMovementEdge);
        }

        if (newMovementEdge == cAirportMovementNodeNull)
        {
            beginNewJourney();

            if (sizeOfOrderTable == 1)
            {
                // 0x4a94a5
                airportMovementEdge = cAirportMovementNodeNull;
                return airplaneApproachTarget(targetZ);
            }

            auto orders = getCurrentOrders();
            auto* order = orders.begin()->as<OrderStopAt>();
            if (order == nullptr)
            {
                airportMovementEdge = cAirportMovementNodeNull;
                return airplaneApproachTarget(targetZ);
            }

            station_id_t orderStationId = order->getStation();

            auto station = StationManager::get(orderStationId);

            if (station == nullptr || !(station->flags & station_flags::flag_6))
            {
                airportMovementEdge = cAirportMovementNodeNull;
                return airplaneApproachTarget(targetZ);
            }

            if (!isPlayerCompany(owner))
            {
                stationId = orderStationId;
                airportMovementEdge = cAirportMovementNodeNull;
                return airplaneApproachTarget(targetZ);
            }

            map_pos3 loc = {
                station->unk_tile_x,
                station->unk_tile_y,
                station->unk_tile_z
            };

            auto tile = Map::TileManager::get(loc);
            for (auto& el : tile)
            {
                auto elStation = el.asStation();
                if (elStation == nullptr)
                    continue;

                if (elStation->baseZ() != loc.z / 4)
                    continue;

                auto airportObject = ObjectManager::get<AirportObject>(elStation->objectId());
                Vehicle train(this);
                uint16_t planeType = train.cars.firstCar.front->getPlaneType();

                if (airportObject->allowed_plane_types & planeType)
                {
                    stationId = orderStationId;
                    airportMovementEdge = cAirportMovementNodeNull;
                    return airplaneApproachTarget(targetZ);
                }

                if (owner == CompanyManager::getControllingId())
                {
                    MessageManager::post(
                        messageType::unableToLandAtAirport,
                        owner,
                        id,
                        orderStationId);
                }

                airportMovementEdge = cAirportMovementNodeNull;
                return airplaneApproachTarget(targetZ);
            }

            // Todo: fail gracefully on tile not found
            assert(false);

            return true;
            // 0x004A938A
        }
        else
        {
            airportMovementEdge = newMovementEdge;
            if (stationId != StationId::null)
            {
                auto station = StationManager::get(stationId);
                station->airportMovementOccupiedEdges |= (1 << airportMovementEdge);
            }
            return airplaneApproachTarget(targetZ);
        }
    }

    namespace WaterMotionFlags
    {
        constexpr uint32_t isStopping = 1 << 0;
        constexpr uint32_t isLeavingDock = 1 << 1;
        constexpr uint32_t hasReachedDock = 1 << 16;
        constexpr uint32_t hasReachedADestination = 1 << 17;
    }

    // 0x004A9649
    bool VehicleHead::updateWater()
    {
        Vehicle2* vehType2 = vehicleUpdate_2;
        if (vehType2->currentSpeed >= 5.0_mph)
        {
            vehicleUpdate_var_1136130 = 0x4000;
        }
        else
        {
            vehicleUpdate_var_1136130 = 0x2000;
        }

        Vehicle train(this);
        train.cars.firstCar.body->sub_4AAB0B();

        if (status == Status::stopped)
        {
            if (var_0C & Flags0C::commandStop)
            {
                return true;
            }

            if (stationId != StationId::null)
            {
                vehType2->currentSpeed = 0.0_mph;
                setStationVisitedTypes();
                checkIfAtOrderStation();
                updateLastJourneyAverageSpeed();
                beginUnloading();
                return true;
            }
        }

        if (var_0C & Flags0C::commandStop)
        {
            if (!(updateWaterMotion(WaterMotionFlags::isStopping) & WaterMotionFlags::hasReachedADestination))
            {
                return true;
            }

            status = Status::stopped;
            vehType2->currentSpeed = 0.0_mph;
            vehType2->var_5A = 0;
            return true;
        }

        if (status == Status::unloading)
        {
            updateUnloadCargo();
            return true;
        }
        else if (status == Status::loading)
        {
            if (updateLoadCargo())
            {
                return true;
            }

            beginNewJourney();
            advanceToNextRoutableOrder();
            status = Status::unk_2;
            status = sub_427BF2();
            updateWaterMotion(WaterMotionFlags::isLeavingDock);
            produceLeavingDockSound();
            return true;
        }
        else
        {
            status = Status::unk_2;
            status = sub_427BF2();
            advanceToNextRoutableOrder();
            if (!(updateWaterMotion(0) & WaterMotionFlags::hasReachedDock))
            {
                return true;
            }

            if (var_0C & Flags0C::commandStop)
            {
                status = Status::stopped;
                vehType2->currentSpeed = 0.0_mph;
                vehType2->var_5A = 0;
                return true;
            }

            vehType2->currentSpeed = 0.0_mph;
            setStationVisitedTypes();
            checkIfAtOrderStation();
            updateLastJourneyAverageSpeed();
            beginUnloading();
            return true;
        }
    }

    /** 0x00427122
     * Seems to work out where to land or something like that.
     *  manhattanDistance = regs.ebp
     *  targetZ = regs.dx
     *  targetYaw = regs.bl
     *  airportFlags = vehicleUpdate_var_525BB0
     */
    std::tuple<uint32_t, uint16_t, uint8_t> VehicleHead::sub_427122()
    {
        vehicleUpdate_var_525BB0 = 0;
        station_id_t targetStationId = StationId::null;
        std::optional<Map::map_pos3> targetPos{};
        if (stationId == StationId::null)
        {
            auto orders = getCurrentOrders();
            auto curOrder = orders.begin();
            auto stationOrder = curOrder->as<OrderStation>();
            if (curOrder->is<OrderRouteWaypoint>() || !(curOrder->hasFlag(OrderFlags::HasStation)) || stationOrder == nullptr)
            {
                targetStationId = stationId;
            }
            else
            {
                targetStationId = stationOrder->getStation();
            }
        }
        else
        {
            if (airportMovementEdge == cAirportMovementNodeNull)
            {
                targetStationId = stationId;
            }
            else
            {
                auto station = StationManager::get(stationId);
                if (!(station->flags & station_flags::flag_6))
                {
                    targetStationId = stationId;
                }
                else
                {
                    auto [flags, pos] = airportGetMovementEdgeTarget(stationId, airportMovementEdge);
                    vehicleUpdate_var_525BB0 = flags;
                    targetPos = pos;
                }
            }
        }

        if (!targetPos)
        {
            if (targetStationId == StationId::null)
            {
                targetPos = map_pos3{ 6143, 6143, 960 };
            }
            else
            {
                auto station = StationManager::get(targetStationId);
                targetPos = map_pos3{ station->x, station->y, 960 };
                if (station->flags & station_flags::flag_6)
                {
                    targetPos = map_pos3{ station->unk_tile_x, station->unk_tile_y, 960 };
                }
            }
        }

        auto xDiff = targetPos->x - x;
        auto yDiff = targetPos->y - y;

        auto targetYaw = calculateYaw1FromVectorPlane(xDiff, yDiff);

        // manhattan distance to target
        auto manhattanDistance = Math::Vector::manhattanDistance(map_pos{ x, y }, *targetPos);

        // Manhatten distance, targetZ, targetYaw
        return std::make_tuple(manhattanDistance, targetPos->z, targetYaw);
    }

    // 0x00427214 returns next movement edge or -2 if no valid edge or -1 for in flight
    uint8_t VehicleHead::airportGetNextMovementEdge(uint8_t curEdge)
    {
        auto station = StationManager::get(stationId);

        map_pos3 loc = {
            station->unk_tile_x,
            station->unk_tile_y,
            station->unk_tile_z
        };

        auto tile = TileManager::get(loc);

        for (auto& el : tile)
        {
            auto elStation = el.asStation();
            if (elStation == nullptr)
                continue;

            if (elStation->baseZ() != loc.z / 4)
                continue;

            auto airportObject = ObjectManager::get<AirportObject>(elStation->objectId());

            if (curEdge == cAirportMovementNodeNull)
            {
                for (uint8_t movementEdge = 0; movementEdge < airportObject->numMovementEdges; movementEdge++)
                {
                    const auto& transition = airportObject->movementEdges[movementEdge];
                    if (!(airportObject->movementNodes[transition.curNode].flags & AirportMovementNodeFlags::flag2))
                    {
                        continue;
                    }

                    if (station->airportMovementOccupiedEdges & transition.mustBeClearEdges)
                    {
                        continue;
                    }

                    if (transition.atLeastOneClearEdges == 0)
                    {
                        return movementEdge;
                    }

                    auto occupiedAreas = station->airportMovementOccupiedEdges & transition.atLeastOneClearEdges;
                    if (occupiedAreas == transition.atLeastOneClearEdges)
                    {
                        continue;
                    }

                    return movementEdge;
                }
                return -2;
            }
            else
            {
                uint8_t targetNode = airportObject->movementEdges[curEdge].nextNode;
                if (status == Status::takingOff && airportObject->movementNodes[targetNode].flags & AirportMovementNodeFlags::takeoffEnd)
                {
                    return cAirportMovementNodeNull;
                }
                // 0x4272A5
                Vehicle train(this);
                auto vehObject = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->object_id);
                if (vehObject->flags & FlagsE0::isHelicopter)
                {
                    for (uint8_t movementEdge = 0; movementEdge < airportObject->numMovementEdges; movementEdge++)
                    {
                        const auto& transition = airportObject->movementEdges[movementEdge];

                        if (transition.curNode != targetNode)
                        {
                            continue;
                        }

                        if (airportObject->movementNodes[transition.nextNode].flags & AirportMovementNodeFlags::takeoffBegin)
                        {
                            continue;
                        }

                        if (station->airportMovementOccupiedEdges & transition.mustBeClearEdges)
                        {
                            continue;
                        }

                        if (transition.atLeastOneClearEdges == 0)
                        {
                            return movementEdge;
                        }

                        auto occupiedAreas = station->airportMovementOccupiedEdges & transition.atLeastOneClearEdges;
                        if (occupiedAreas == transition.atLeastOneClearEdges)
                        {
                            continue;
                        }
                        return movementEdge;
                    }

                    return -2;
                }
                else
                {
                    for (uint8_t movementEdge = 0; movementEdge < airportObject->numMovementEdges; movementEdge++)
                    {
                        const auto& transition = airportObject->movementEdges[movementEdge];
                        if (transition.curNode != targetNode)
                        {
                            continue;
                        }

                        if (airportObject->movementNodes[transition.nextNode].flags & AirportMovementNodeFlags::heliTakeoffBegin)
                        {
                            continue;
                        }

                        if (station->airportMovementOccupiedEdges & transition.mustBeClearEdges)
                        {
                            continue;
                        }

                        if (transition.atLeastOneClearEdges == 0)
                        {
                            return movementEdge;
                        }

                        auto occupiedAreas = station->airportMovementOccupiedEdges & transition.atLeastOneClearEdges;
                        if (occupiedAreas == transition.atLeastOneClearEdges)
                        {
                            continue;
                        }

                        return movementEdge;
                    }
                    return -2;
                }
            }
        }

        // Tile not found. Todo: fail gracefully
        assert(false);
        return cAirportMovementNodeNull;
    }

    // 0x00426E26
    std::pair<uint32_t, Map::map_pos3> VehicleHead::airportGetMovementEdgeTarget(station_id_t targetStation, uint8_t curEdge)
    {
        auto station = StationManager::get(targetStation);

        map_pos3 staionLoc = {
            station->unk_tile_x,
            station->unk_tile_y,
            station->unk_tile_z
        };

        auto tile = TileManager::get(staionLoc);

        for (auto& el : tile)
        {
            auto elStation = el.asStation();
            if (elStation == nullptr)
                continue;

            if (elStation->baseZ() != staionLoc.z / 4)
                continue;

            auto airportObject = ObjectManager::get<AirportObject>(elStation->objectId());

            auto destinationNode = airportObject->movementEdges[curEdge].nextNode;

            map_pos loc2 = {
                static_cast<int16_t>(airportObject->movementNodes[destinationNode].x - 16),
                static_cast<int16_t>(airportObject->movementNodes[destinationNode].y - 16)
            };
            loc2 = Math::Vector::rotate(loc2, elStation->rotation());
            auto airportFlags = airportObject->movementNodes[destinationNode].flags;

            loc2.x += 16 + staionLoc.x;
            loc2.y += 16 + staionLoc.y;

            map_pos3 loc = { loc2.x, loc2.y, static_cast<int16_t>(airportObject->movementNodes[destinationNode].z + staionLoc.z) };

            if (!(airportFlags & AirportMovementNodeFlags::taxiing))
            {
                loc.z = staionLoc.z + 255;
                if (!(airportFlags & AirportMovementNodeFlags::inFlight))
                {
                    loc.z = 960;
                }
            }

            return std::make_pair(airportFlags, loc);
        }

        // Tile not found. Todo: fail gracefully
        assert(false);
        // Flags, location
        return std::make_pair(0, Map::map_pos3{ 0, 0, 0 });
    }

    // 0x004B980A
    void VehicleHead::tryCreateInitialMovementSound()
    {
        if (status != Status::unk_2)
        {
            return;
        }

        if (vehicleUpdate_initialStatus != Status::stopped && vehicleUpdate_initialStatus != Status::unk_3)
        {
            return;
        }

        Vehicle train(this);
        auto* vehObj = train.cars.firstCar.body->object();
        if (vehObj != nullptr && vehObj->numStartSounds != 0)
        {
            auto numSounds = vehObj->numStartSounds & NumStartSounds::mask;
            if (vehObj->numStartSounds & NumStartSounds::hasCrossingWhistle)
            {
                // remove the crossing whistle from available sounds to play
                numSounds = std::max(numSounds - 1, 1);
            }
            auto randSoundIndex = gPrng().randNext(numSounds - 1);
            auto randSoundId = Audio::makeObjectSoundId(vehObj->startSounds[randSoundIndex]);
            Vehicle2* veh2 = vehicleUpdate_2;
            auto tileHeight = TileManager::getHeight({ veh2->x, veh2->y });
            auto volume = 0;
            if (veh2->z < tileHeight.landHeight)
            {
                volume = -1500;
            }
            Audio::playSound(randSoundId, { veh2->x, veh2->y, static_cast<int16_t>(veh2->z + 22) }, volume, 22050);
        }
    }

    // 0x004B996F
    void VehicleHead::setStationVisitedTypes()
    {
        auto station = StationManager::get(stationId);
        station->var_3B2 |= (1 << static_cast<uint8_t>(vehicleType));
    }

    // 0x004B9987
    void VehicleHead::checkIfAtOrderStation()
    {
        OrderRingView orders(orderTableOffset, currentOrder);
        auto curOrder = orders.begin();
        auto* orderStation = curOrder->as<OrderStation>();
        if (orderStation == nullptr)
        {
            return;
        }

        if (orderStation->getStation() != stationId)
        {
            return;
        }

        curOrder++;
        currentOrder = curOrder->getOffset() - orderTableOffset;
        Ui::WindowManager::sub_4B93A5(id);
    }

    // 0x004BACAF
    void VehicleHead::updateLastJourneyAverageSpeed()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004BACAF, regs);
    }

    // 0x004B99E1
    void VehicleHead::beginUnloading()
    {
        var_5F &= ~Flags5F::unk_0;
        status = Status::unloading;
        var_56 = 10;
        var_58 = 0;

        Vehicle train(this);
        for (auto& car : train.cars)
        {
            for (auto& carComponent : car)
            {
                carComponent.front->var_5F |= Flags5F::unk_0;
                carComponent.back->var_5F |= Flags5F::unk_0;
                carComponent.body->var_5F |= Flags5F::unk_0;
            }
        }
    }

    // 0x00426CA4
    void VehicleHead::movePlaneTo(const Map::map_pos3& newLoc, const uint8_t newYaw, const Pitch newPitch)
    {
        Vehicle train(this);
        moveTo({ newLoc.x, newLoc.y, newLoc.z });
        tile_x = 0;

        train.veh1->moveTo({ newLoc.x, newLoc.y, newLoc.z });
        train.veh1->tile_x = 0;

        train.veh2->moveTo({ newLoc.x, newLoc.y, newLoc.z });
        train.veh2->tile_x = 0;

        // The first bogie of the plane is the shadow of the plane
        auto* shadow = train.cars.firstCar.front;
        shadow->invalidateSprite();
        auto height = coord_t{ TileManager::getHeight(newLoc) };
        shadow->moveTo({ newLoc.x, newLoc.y, height });
        shadow->sprite_yaw = newYaw;
        shadow->sprite_pitch = Pitch::flat;
        shadow->tile_x = 0;
        shadow->invalidateSprite();

        auto* body = train.cars.firstCar.body;
        body->invalidateSprite();
        body->moveTo({ newLoc.x, newLoc.y, newLoc.z });
        body->sprite_yaw = newYaw;
        body->sprite_pitch = newPitch;
        body->tile_x = 0;
        body->invalidateSprite();
    }

    // 0x00427C05
    // Input flags:
    // bit 0  : commandedToStop
    // bit 1  : isLeavingDock
    // Output flags:
    // bit 16 : reachedDock
    // bit 17 : reachedADestination
    uint32_t VehicleHead::updateWaterMotion(uint32_t flags)
    {
        Vehicle2* veh2 = vehicleUpdate_2;

        // updates the current boats position and sets flags about position
        auto tile = TileManager::get(Map::map_pos{ veh2->x, veh2->y });
        surface_element* surface = tile.surface();

        if (surface != nullptr)
        {
            auto waterHeight = surface->water();
            if (waterHeight != 0)
            {
                if (surface->hasHighTypeFlag())
                {
                    surface->setHighTypeFlag(false);
                    surface->setVar6SLR5(0);
                }
                surface->setIndustry(0);
                surface->setType6Flag(true);
            }
        }

        auto targetSpeed = 5_mph;
        if (stationId == StationId::null)
        {
            if (!(flags & WaterMotionFlags::isStopping))
            {
                if (!(veh2->var_73 & Flags73::isBrokenDown))
                {
                    targetSpeed = veh2->maxSpeed;
                }
            }
        }

        if (targetSpeed == veh2->currentSpeed)
        {
            veh2->var_5A = 2;
        }
        else if (targetSpeed < veh2->currentSpeed)
        {
            veh2->var_5A = 2;
            auto decelerationRate = 1.0_mph;
            if (veh2->currentSpeed >= 50.0_mph)
            {
                decelerationRate = 3.0_mph;
            }
            veh2->var_5A = 3;
            auto newSpeed = std::max(veh2->currentSpeed - decelerationRate, 0.0_mph);
            veh2->currentSpeed = std::max<Speed32>(targetSpeed, newSpeed);
        }
        else
        {
            veh2->var_5A = 1;
            veh2->currentSpeed = std::min<Speed32>(targetSpeed, veh2->currentSpeed + 0.333333_mph);
        }

        auto manhattanDistance = Math::Vector::manhattanDistance(map_pos{ x, y }, map_pos{ veh2->x, veh2->y });
        auto targetTolerance = 3;
        if (veh2->currentSpeed >= 20.0_mph)
        {
            targetTolerance = 16;
            if (veh2->currentSpeed > 70.0_mph)
            {
                targetTolerance = 24;
            }
        }

        if ((flags & WaterMotionFlags::isLeavingDock) || manhattanDistance <= targetTolerance)
        {
            flags |= WaterMotionFlags::hasReachedADestination;
            if (stationId != StationId::null && !(flags & WaterMotionFlags::isLeavingDock))
            {
                flags |= WaterMotionFlags::hasReachedDock;
            }
            if (flags & WaterMotionFlags::isStopping)
            {
                return flags;
            }

            OrderRingView orders(orderTableOffset, currentOrder);
            auto curOrder = orders.begin();
            auto waypoint = curOrder->as<OrderRouteWaypoint>();
            if (waypoint != nullptr)
            {
                auto point = waypoint->getWaypoint();
                if (point.x == (x & 0xFFE0) && point.y == (y & 0xFFE0))
                {
                    currentOrder = (++curOrder)->getOffset() - orderTableOffset;
                    Ui::WindowManager::sub_4B93A5(id);
                }
            }

            if (!(flags & WaterMotionFlags::isLeavingDock) && stationId != StationId::null)
            {
                return flags;
            }

            if (stationId != StationId::null)
            {
                auto targetTile = TileManager::get(Map::map_pos{ tile_x, tile_y });
                station_element* station = nullptr;
                for (auto& el : targetTile)
                {
                    station = el.asStation();
                    if (station == nullptr)
                    {
                        continue;
                    }
                    if (station->isGhost() || station->isFlag5())
                    {
                        continue;
                    }
                    if (station->baseZ() == tile_base_z)
                    {
                        station->setFlag6(false);
                        stationId = StationId::null;
                        break;
                    }
                }
            }
            auto [newStationId, headTarget, stationTarget] = sub_427FC9();
            moveTo({ headTarget.x, headTarget.y, 32 });

            if (newStationId != StationId::null)
            {
                stationId = newStationId;
                tile_x = stationTarget.x;
                tile_y = stationTarget.y;
                tile_base_z = stationTarget.z / 4;

                auto targetTile = TileManager::get(Map::map_pos{ tile_x, tile_y });
                station_element* station = nullptr;
                for (auto& el : targetTile)
                {
                    station = el.asStation();
                    if (station == nullptr)
                    {
                        continue;
                    }
                    if (station->isGhost() || station->isFlag5())
                    {
                        continue;
                    }
                    if (station->baseZ() == tile_base_z)
                    {
                        station->setFlag6(true);
                        break;
                    }
                }
            }
        }

        auto targetYaw = calculateYaw4FromVector(x - veh2->x, y - veh2->y);
        if (targetYaw != veh2->sprite_yaw)
        {
            if (((targetYaw - veh2->sprite_yaw) & 0x3F) > 0x20)
            {
                veh2->sprite_yaw--;
            }
            else
            {
                veh2->sprite_yaw++;
            }
            veh2->sprite_yaw &= 0x3F;
        }

        Vehicle1* veh1 = vehicleUpdate_1;
        auto [newVeh1Pos, newVeh2Pos] = calculateNextPosition(veh2->sprite_yaw, { veh2->x, veh2->y }, veh1, veh2->currentSpeed);

        veh1->var_4E = newVeh1Pos.x;
        veh1->var_50 = newVeh1Pos.y;

        map_pos3 newLocation = { newVeh2Pos.x, newVeh2Pos.y, veh2->z };
        moveBoatTo(newLocation, veh2->sprite_yaw, Pitch::flat);

        return flags;
    }

    // 0x00427B70
    void VehicleHead::moveBoatTo(const map_pos3& newLoc, const uint8_t yaw, const Pitch pitch)
    {
        Vehicle train(this);
        train.veh1->moveTo({ newLoc.x, newLoc.y, newLoc.z });
        train.veh1->tile_x = 0;
        train.veh2->moveTo({ newLoc.x, newLoc.y, newLoc.z });
        train.veh2->tile_x = 0;
        train.cars.firstCar.body->invalidateSprite();
        train.cars.firstCar.body->moveTo({ newLoc.x, newLoc.y, newLoc.z });
        train.cars.firstCar.body->sprite_yaw = yaw;
        train.cars.firstCar.body->sprite_pitch = pitch;
        train.cars.firstCar.body->tile_x = 0;
        train.cars.firstCar.body->invalidateSprite();
    }

    // 0x004B9A2A
    void VehicleHead::updateUnloadCargo()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004B9A2A, regs);
    }

    // 0x004BA142 returns false when loaded
    bool VehicleHead::updateLoadCargo()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        return call(0x004BA142, regs) & (1 << 8);
    }

    // 0x004BAC74
    void VehicleHead::beginNewJourney()
    {
        // Set initial position for updateLastJourneyAverageSpeed
        var_73 = scenarioTicks();
        Vehicle train(this);
        var_6F = train.veh2->x;
        var_71 = train.veh2->y;
        var_5F |= Flags5F::unk_3;
    }

    // 0x004707C0
    void VehicleHead::advanceToNextRoutableOrder()
    {
        if (sizeOfOrderTable == 1)
        {
            return;
        }
        OrderRingView orders(orderTableOffset, currentOrder);
        for (auto& order : orders)
        {
            if (order.hasFlag(OrderFlags::IsRoutable))
            {
                auto newOrder = order.getOffset() - orderTableOffset;
                if (newOrder != currentOrder)
                {
                    currentOrder = newOrder;
                    Ui::WindowManager::sub_4B93A5(id);
                }
                return;
            }
        }
    }

    // 0x00427BF2
    Status VehicleHead::sub_427BF2()
    {
        return stationId == StationId::null ? Status::unk_2 : Status::approaching;
    }

    // 0x0042843E
    void VehicleHead::produceLeavingDockSound()
    {
        Vehicle train(this);
        auto* vehObj = train.cars.firstCar.body->object();
        if (vehObj != nullptr && vehObj->numStartSounds != 0)
        {
            auto randSoundIndex = gPrng().randNext((vehObj->numStartSounds & NumStartSounds::mask) - 1);
            auto randSoundId = Audio::makeObjectSoundId(vehObj->startSounds[randSoundIndex]);
            Vehicle2* veh2 = vehicleUpdate_2;
            Audio::playSound(randSoundId, { veh2->x, veh2->y, static_cast<int16_t>(veh2->z + 22) }, 0, 22050);
        }
    }

    // 0x00427FC9
    std::tuple<station_id_t, Map::map_pos, Map::map_pos3> VehicleHead::sub_427FC9()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x00427FC9, regs);
        Map::map_pos headTarget = { regs.ax, regs.cx };
        Map::map_pos3 stationTarget = { regs.di, regs.bp, regs.dl };
        return std::make_tuple(regs.bx, headTarget, stationTarget);
    }

    // 0x0042750E
    void VehicleHead::produceTouchdownAirportSound()
    {
        Vehicle train(this);
        auto* vehObj = train.cars.firstCar.body->object();
        if (vehObj != nullptr && vehObj->numStartSounds != 0)
        {

            auto randSoundIndex = gPrng().randNext((vehObj->numStartSounds & NumStartSounds::mask) - 1);
            auto randSoundId = Audio::makeObjectSoundId(vehObj->startSounds[randSoundIndex]);

            Vehicle2* veh2 = vehicleUpdate_2;
            Audio::playSound(randSoundId, { veh2->x, veh2->y, static_cast<int16_t>(veh2->z + 22) }, 0, 22050);
        }
    }

    // 0x004AD778
    void VehicleHead::sub_4AD778()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004AD778, regs);
    }

    // 0x004AA625
    void VehicleHead::sub_4AA625()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004AA625, regs);
    }

    // 0x004ACEE7
    std::tuple<uint8_t, uint8_t, station_id_t> VehicleHead::sub_4ACEE7(uint32_t unk1, uint32_t var_113612C)
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        regs.eax = unk1;
        regs.ebx = var_113612C;
        call(0x004ACEE7, regs);
        // status, flags, stationId
        return std::make_tuple(static_cast<uint8_t>(regs.al), static_cast<uint8_t>(regs.ah), static_cast<station_id_t>(regs.bp));
    }

    // 0x004AC1C2
    bool VehicleHead::sub_4AC1C2()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        return call(0x004AC1C2, regs) & (1 << 8);
    }

    // 0x004AC0A3
    bool VehicleHead::sub_4AC0A3()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        return call(0x004AC0A3, regs) & (1 << 8);
    }

    // 0x004ACCDC
    bool VehicleHead::sub_4ACCDC()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        return call(0x004ACCDC, regs) & (1 << 8);
    }

    // 0x004AD93A
    void VehicleHead::sub_4AD93A()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004AD93A, regs);
    }

    static station_id_t tryFindStationAt(VehicleBogie* bogie)
    {
        map_pos loc{ bogie->tile_x, bogie->tile_y };
        auto baseZ = bogie->tile_base_z;
        auto direction = bogie->var_2C & 3;
        auto trackId = (bogie->var_2C >> 3) & 0x3F;

        bool findStation = false;
        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* elTrack = el.asTrack();
            auto* elStation = el.asStation();
            if (findStation && elStation != nullptr)
            {
                findStation = false;
                if (elStation->isFlag5() || elStation->isGhost())
                    continue;

                return elStation->stationId();
            }
            if (elTrack == nullptr)
                continue;

            if (elTrack->baseZ() != baseZ)
                continue;

            if (elTrack->unkDirection() != direction)
                continue;

            if (elTrack->trackId() != trackId)
                continue;

            if (!elTrack->hasStationElement())
                continue;

            findStation = true;
        }
        return StationId::null;
    }

    // 0x004B6669
    char* VehicleHead::generateCargoTotalString(char* buffer)
    {
        uint32_t cargoTotals[ObjectManager::getMaxObjects(object_type::cargo)]{};
        Vehicles::Vehicle train(this);
        for (auto& car : train.cars)
        {
            auto front = car.front;
            auto body = car.body;
            if (front->cargo_type != 0xFF)
            {
                cargoTotals[front->cargo_type] += front->secondaryCargoQty;
            }
            if (body->cargo_type != 0xFF)
            {
                cargoTotals[body->cargo_type] += body->primaryCargoQty;
            }
        }

        bool hasCargo = false;
        for (size_t cargoType = 0; cargoType < ObjectManager::getMaxObjects(object_type::cargo); ++cargoType)
        {
            auto cargoTotal = cargoTotals[cargoType];
            if (cargoTotal == 0)
            {
                continue;
            }

            // On all but first loop insert a ", "
            if (hasCargo)
            {
                *buffer++ = ',';
                *buffer++ = ' ';
            }
            hasCargo = true;
            auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
            auto unitNameFormat = cargoTotal == 1 ? cargoObj->unit_name_singular : cargoObj->unit_name_plural;
            FormatArguments args{};
            args.push(cargoTotal);
            buffer = StringManager::formatString(buffer, unitNameFormat, &args);
        }

        if (!hasCargo)
        {
            buffer = StringManager::formatString(buffer, StringIds::cargo_empty_2);
        }

        return buffer;
    }

    // 0x004BABAD
    // Manual control has much broader detection of when at a station
    // it is defined as stationary with at least one bogie at the station
    station_id_t VehicleHead::manualFindTrainStationAtLocation()
    {
        Vehicle train(this);
        for (auto& car : train.cars)
        {
            for (auto& component : car)
            {
                auto foundStation = tryFindStationAt(component.front);
                if (foundStation != StationId::null)
                {
                    return foundStation;
                }
                foundStation = tryFindStationAt(component.back);
                if (foundStation != StationId::null)
                {
                    return foundStation;
                }
            }
        }
        return StationId::null;
    }

    // 0x004ADB47
    void VehicleHead::sub_4ADB47(bool unk)
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        regs.eax = unk ? 1 : 0;
        call(0x004ADB47, regs);
    }

    // 0x004BADE4
    bool VehicleHead::isOnExpectedRoadOrTrack()
    {
        Vehicle train(this);
        Vehicle2* veh = train.veh2;
        map_pos loc = {
            veh->tile_x,
            veh->tile_y
        };
        auto baseZ = veh->tile_base_z;

        auto tile = TileManager::get(loc);
        if (veh->mode == TransportMode::road)
        {
            for (auto& el : tile)
            {
                auto elRoad = el.asRoad();
                if (elRoad == nullptr)
                    continue;

                auto heightDiff = std::abs(elRoad->baseZ() - baseZ);
                if (heightDiff > 4)
                    continue;

                if (elRoad->isGhost() || elRoad->isFlag5())
                    continue;

                if (elRoad->roadId() != ((veh->var_2C >> 3) & 0xF))
                    continue;

                return true;
            }
            return false;
        }
        else
        {
            for (auto& el : tile)
            {
                auto elTrack = el.asTrack();
                if (elTrack == nullptr)
                    continue;

                auto heightDiff = std::abs(elTrack->baseZ() - baseZ);
                if (heightDiff > 4)
                    continue;

                if (elTrack->isGhost() || elTrack->isFlag5())
                    continue;

                if (elTrack->unkDirection() != (veh->var_2C & 0x3))
                    continue;

                if (elTrack->trackId() != ((veh->var_2C >> 3) & 0x3F))
                    continue;

                return true;
            }
            return false;
        }
    }

    OrderRingView Vehicles::VehicleHead::getCurrentOrders() const
    {
        return OrderRingView(orderTableOffset, currentOrder);
    }
}
