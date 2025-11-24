#include "MessageManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "Tutorial.h"
#include "Vehicle.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> _vehicleUpdate_var_113612C; // Speed
    static loco_global<int32_t, 0x01136130> _vehicleUpdate_var_1136130; // Speed

    constexpr const uint8_t kBrakeLightTimeout = 7;

    // values are pre *256 for maths
    // Lateral force due to gravity due to Pitch possibly SIN(angle) * 256 * 256
    static constexpr std::array<int32_t, 13> _500170{
        0,
        -6645,
        -13223,
        -19182,
        -24963,
        6645,
        13223,
        19182,
        24963,
        -11290,
        11290,
        -21628,
        21628,
    };

    // 0x004A9BA0
    static bool shouldStartWheelSlipping(const Vehicle& train, VehicleBogie& frontBogie)
    {
        const auto* vehObject = ObjectManager::get<VehicleObject>(frontBogie.objectId);
        if (!vehObject->hasFlags(VehicleObjectFlags::canWheelslip) || vehObject->power == 0)
        {
            return false;
        }

        if (train.head->status != Status::travelling)
        {
            return false;
        }

        if (train.veh2->motorState == MotorState::coasting || train.veh2->currentSpeed > 10.0_mph)
        {
            return false;
        }

        // Only start wheel slipping if it is not wheel slipping already
        if (frontBogie.wheelSlipping != 0)
        {
            return false;
        }

        if (Tutorial::state() != Tutorial::State::none)
        {
            return false;
        }

        if (train.head->hasVehicleFlags(VehicleFlags::manualControl) && train.head->manualPower <= 10)
        {
            return false;
        }

        const auto tot1 = 128ULL * vehObject->power * train.veh2->totalWeight;
        const auto tot2 = frontBogie.totalCarWeight * train.veh2->totalPower;
        auto fraction = tot2 == 0 ? tot1 : tot1 / tot2;
        fraction = std::min(fraction, 2'000ULL);
        if (fraction < static_cast<uint16_t>(gPrng1().randNext(0xFFFF)))
        {
            return false;
        }
        if (frontBogie.mode == TransportMode::rail)
        {
            const auto* trackObj = ObjectManager::get<TrackObject>(frontBogie.trackType);
            if (trackObj->hasFlags(TrackObjectFlags::unk_01))
            {
                return false;
            }
            if (trackObj->hasFlags(TrackObjectFlags::unk_00))
            {
                if (frontBogie.isOnRackRail())
                {
                    return false;
                }
            }
        }
        else
        {
            if (frontBogie.trackType == 0xFF)
            {
                return false;
            }
            const auto* roadObj = ObjectManager::get<RoadObject>(frontBogie.trackType);
            if (roadObj->hasFlags(RoadObjectFlags::unk_04))
            {
                return false;
            }

            if (roadObj->hasFlags(RoadObjectFlags::unk_05))
            {
                if (frontBogie.isOnRackRail())
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool Vehicle2::has73Flags(Flags73 flagsToTest) const
    {
        return (var_73 & flagsToTest) != Flags73::none;
    }

    // 0x004A9B0B
    bool Vehicle2::update()
    {
        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }
        Vehicle train(head);

        motorState = MotorState::accelerating;
        const auto speedDiff = currentSpeed - train.veh1->targetSpeed;
        if (speedDiff > 0.0_mph)
        {
            motorState = MotorState::braking;
            const auto newSpeed = currentSpeed - (currentSpeed / 64 + 0.18311_mph);
            currentSpeed = std::max(newSpeed, std::max<Speed32>(train.veh1->targetSpeed, 5.0_mph));
            return sub_4A9F20();
        }

        if (!train.head->hasVehicleFlags(VehicleFlags::manualControl))
        {
            if (speedDiff >= -1.5_mph)
            {
                motorState = MotorState::coasting;
            }
            if (currentSpeed == 0.0_mph)
            {
                motorState = MotorState::stopped;
            }
        }

        train.cars.applyToComponents([](auto& component) {
            // If the vehicle is wheel slipping, then its wheel slipping value is incremented.
            if (component.wheelSlipping != 0)
            {
                component.wheelSlipping++;
                if (component.wheelSlipping >= kWheelSlippingDuration)
                {
                    component.wheelSlipping = 0;
                }
            }
        });

        bool isOnRackRail = true; // Note has been inverted
        bool isWheelSlipping = false;
        int32_t ebp = 0;
        for (auto& car : train.cars)
        {
            auto* frontBogie = car.front;
            if (shouldStartWheelSlipping(train, *frontBogie))
            {
                car.applyToComponents([](auto& component) {
                    component.wheelSlipping = 1;
                });
            }

            if (frontBogie->wheelSlipping != 0)
            {
                isWheelSlipping = true;
            }
            if (_500170[enumValue(frontBogie->spritePitch)] <= -19182)
            {
                const auto* vehObject = ObjectManager::get<VehicleObject>(frontBogie->objectId);
                if (vehObject->power != 0)
                {
                    isOnRackRail &= frontBogie->isOnRackRail();
                }
            }
            ebp += (frontBogie->totalCarWeight * _500170[enumValue(frontBogie->spritePitch)]) >> 8;
        }

        if (!isOnRackRail)
        {
            ebp /= 2;
            if (!train.head->hasVehicleFlags(VehicleFlags::unk_0))
            {
                motorState = MotorState::stoppedOnIncline;
                if (currentSpeed <= 3.0_mph && train.head->owner == CompanyManager::getControllingId())
                {
                    MessageManager::post(MessageType::vehicleSlipped, train.head->owner, enumValue(train.head->id), 0xFFFF);
                }
            }
        }
        if (isOnRackRail && !isWheelSlipping)
        {
            // calculating power when it's broken down
            const auto power = has73Flags(Flags73::isBrokenDown) ? totalPower / 4 : totalPower;

            if (train.head->hasVehicleFlags(VehicleFlags::manualControl))
            {
                const auto manualSpeed = train.head->manualPower;
                if (manualSpeed < 0)
                {
                    if (manualSpeed <= -10)
                    {
                        motorState = MotorState::braking;
                    }
                    else
                    {
                        motorState = MotorState::coasting;
                    }
                }
                else
                {
                    if (manualSpeed >= 10)
                    {
                        motorState = MotorState::accelerating;
                    }
                    else
                    {
                        motorState = MotorState::coasting;
                    }
                }
                ebp += ((power * 2048) * manualSpeed) / (totalWeight * 40);
            }
            else
            {
                ebp += (power * 2048) / totalWeight;
            }
        }
        const auto speedSquare = toSpeed16(currentSpeed).getRaw() * toSpeed16(currentSpeed).getRaw();
        ebp -= speedSquare;

        auto speedAdjustment = std::min(Speed32(ebp), 0.5_mph);
        // Vanilla did some funky maths that interpreted signed speeds as unsigned
        // to behave similar we will set the speedAdjustment to 0.5_mph when negative speed
        if (currentSpeed < 0.0_mph)
        {
            speedAdjustment = 0.5_mph;
        }

        auto newSpeed = speedAdjustment + currentSpeed;
        if (speedAdjustment < 0.0_mph)
        {
            auto minSpeed = 5.0_mph;
            if (train.head->hasVehicleFlags(VehicleFlags::manualControl) || !isOnRackRail)
            {
                minSpeed = 0.0_mph;
            }
            if (currentSpeed >= minSpeed)
            {
                newSpeed = std::max(newSpeed, minSpeed);
            }
        }

        if (!train.head->hasVehicleFlags(VehicleFlags::manualControl))
        {
            // Vanilla did some funky maths that interpreted signed speeds as unsigned
            // to behave similar we always take the train.veh1->targetSpeed on negative speed
            if (newSpeed < 0.0_mph)
            {
                newSpeed = train.veh1->targetSpeed;
            }
            else
            {
                newSpeed = std::min<Speed32>(newSpeed, train.veh1->targetSpeed);
            }
        }
        currentSpeed = newSpeed;

        return sub_4A9F20();
    }

    // 0x004A9F20
    bool Vehicle2::sub_4A9F20()
    {
        Vehicle train(head);

        auto res = updateTrackMotion(_vehicleUpdate_var_113612C, true);
        _vehicleUpdate_var_113612C = _vehicleUpdate_var_113612C - res.remainingDistance;
        _vehicleUpdate_var_1136130 = _vehicleUpdate_var_1136130 - res.remainingDistance;
        if (res.hasFlags(UpdateVar1136114Flags::noRouteFound))
        {
            destroyTrain();
            return false;
        }

        if (res.hasFlags(UpdateVar1136114Flags::unk_m00))
        {
            if (!train.head->hasVehicleFlags(VehicleFlags::manualControl))
            {
                currentSpeed = 0.0_mph;
                motorState = MotorState::stopped;
            }
        }

        if (motorState == MotorState::stoppedOnIncline)
        {
            _vehicleUpdate_var_1136130 = _vehicleUpdate_var_113612C + 0x1388;
        }

        train.head->var_3C -= _vehicleUpdate_var_113612C;
        train.veh1->var_3C -= _vehicleUpdate_var_113612C;

        if (motorState == MotorState::braking)
        {
            if (brakeLightTimeout == 0)
            {
                invalidateSprite();
                train.veh1->invalidateSprite();
                train.tail->invalidateSprite();
                train.cars.applyToComponents([](auto& component) { component.invalidateSprite(); });
            }

            brakeLightTimeout = kBrakeLightTimeout;
            return true;
        }
        else
        {
            if (brakeLightTimeout == 0)
            {
                return true;
            }
            brakeLightTimeout--;
            if (brakeLightTimeout == 0)
            {
                invalidateSprite();
                train.veh1->invalidateSprite();
                train.tail->invalidateSprite();
                train.cars.applyToComponents([](auto& component) { component.invalidateSprite(); });
            }
            return true;
        }
    }
}
