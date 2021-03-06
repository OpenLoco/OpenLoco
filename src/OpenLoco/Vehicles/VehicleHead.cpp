#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../StationManager.h"
#include "../Things/Misc.h"
#include "../Things/ThingManager.h"
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
    static loco_global<VehicleHead*, 0x01136118> vehicleUpdate_head;
    static loco_global<Vehicle1*, 0x0113611C> vehicleUpdate_1;
    static loco_global<Vehicle2*, 0x01136120> vehicleUpdate_2;
    static loco_global<VehicleBogie*, 0x01136124> vehicleUpdate_frontBogie;
    static loco_global<VehicleBogie*, 0x01136128> vehicleUpdate_backBogie;
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<int32_t, 0x01136130> vehicleUpdate_var_1136130; // Speed
    static loco_global<uint8_t, 0x01136237> vehicle_var_1136237;       // var_28 related?
    static loco_global<uint8_t, 0x01136238> vehicle_var_1136238;       // var_28 related?
    static loco_global<Status, 0x0113646C> vehicleUpdate_initialStatus;
    static loco_global<int16_t[128], 0x00503B6A> factorXY503B6A;

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
                    Smoke::create(loc16(v2->x, v2->y, v2->z + 4));
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
                    Audio::playSound(soundId, loc16(v2->x, v2->y, v2->z + 22));
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
            auto* vehObj = ObjectManager::get<vehicle_object>(car.front->object_id);
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
        auto newObject = ObjectManager::get<vehicle_object>(newVehicleTypeId);       //edi
        auto sourceObject = ObjectManager::get<vehicle_object>(sourceVehicleTypeId); // esi

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
        auto vehObject = ObjectManager::get<vehicle_object>(vehicleTypeId);
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
        auto newObject = ObjectManager::get<vehicle_object>(vehicleTypeId);
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

        auto vehicleObject = ObjectManager::get<vehicle_object>(vehType2or6->objectId);
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
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        return (call(0x004A8C11, regs) & (1 << 8)) == 0;
    }

    // 0x004A9051
    bool VehicleHead::updateAir()
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        return (call(0x004A9051, regs) & (1 << 8)) == 0;
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

        auto manhattanDistance = std::abs(x - veh2->x) + std::abs(y - veh2->y);
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

        map_pos3 newLocation = { newVeh2Pos, veh2->z };
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
}
