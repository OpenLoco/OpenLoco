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
#include "../ViewportManager.h"
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
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004B980A, regs);
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
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004B9987, regs);
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
    uint32_t VehicleHead::updateWaterMotion(uint32_t flags)
    {
        // some sort of routing logic
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        regs.eax = flags;
        call(0x00427C05, regs);
        return regs.eax;
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
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x004707C0, regs);
    }

    // 0x00427BF2
    Status VehicleHead::sub_427BF2()
    {
        return stationId == StationId::null ? Status::unk_2 : Status::approaching;
    }

    // 0x0042843E
    void VehicleHead::produceLeavingDockSound()
    {
        // Creates a random sound
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(this);
        call(0x0042843E, regs);
    }
}
