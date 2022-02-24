#include "../CompanyManager.h"
#include "../Interop/Interop.hpp"
#include "../MessageManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Tutorial.h"
#include "Vehicle.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<int32_t, 0x01136130> vehicleUpdate_var_1136130; // Speed
    static loco_global<Speed32, 0x01136134> vehicleUpdate_var_1136134; // Speed
    static loco_global<VehicleHead*, 0x01136118> vehicleUpdate_head;

    // values are pre *256 for maths
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

    // 0x004A9B0B
    bool Vehicle2::update()
    {
        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }

        var_5A = 1;
        const auto speedDiff = currentSpeed - *vehicleUpdate_var_1136134;
        if (speedDiff > 0.0_mph)
        {
            var_5A = 3;
            const auto newSpeed = currentSpeed - (currentSpeed / 64 + 0.18311_mph);
            currentSpeed = std::clamp(newSpeed, *vehicleUpdate_var_1136134, 50.0_mph);
            // 0x004A9F20
        }

        if (!((*vehicleUpdate_head)->var_0C & Flags0C::manualControl))
        {
            if (speedDiff >= -1.5_mph)
            {
                var_5A = 2;
            }
            if (currentSpeed == 0.0_mph)
            {
                var_5A = 0;
            }
        }

        Vehicle train(head);
        for (auto& car : train.cars)
        {
            for (auto& carComponent : car)
            {
                if (carComponent.front->var_5E != 0)
                {
                    carComponent.front->var_5E++;
                    carComponent.front->var_5E &= 0x3F;
                }
                if (carComponent.back->var_5E != 0)
                {
                    carComponent.back->var_5E++;
                    carComponent.back->var_5E &= 0x3F;
                }
                if (carComponent.body->var_5E != 0)
                {
                    carComponent.body->var_5E++;
                    carComponent.body->var_5E &= 0x3F;
                }
            }
        }

        bool isOnRackRail = true; // Note has been inverted
        uint8_t dh = 0;
        uint32_t ebp = 0;
        for (auto& car : train.cars)
        {
            auto* frontBogie = car.front;
            const auto* vehObject = ObjectManager::get<VehicleObject>(frontBogie->objectId);
            if (train.head->status == Status::travelling
                && var_5A != 2
                && vehObject->power != 0
                && currentSpeed <= 10.0_mph
                && frontBogie->var_5E == 0
                && !(vehObject->flags & FlagsE0::isHelicopter)
                && Tutorial::state() == Tutorial::State::none
                && (!(train.head->var_0C & Flags0C::manualControl)
                    || train.head->var_6E > 10))
            {
                const auto tot1 = vehObject->power * totalWeight * 128ULL;
                const auto tot2 = frontBogie->var_52 * totalPower;
                auto fraction = tot2 == 0 ? tot1 : tot1 / tot2;
                fraction = std::min(fraction, 2'000ULL);
                if (fraction <= gPrng().randNext(0xFFFF))
                {
                    if (frontBogie->mode == TransportMode::rail)
                    {
                        const auto* trackObj = ObjectManager::get<TrackObject>(frontBogie->trackType);
                        if (trackObj->flags & Flags22::unk_01)
                        {
                            // 4a9d08 continue
                        }
                        if (trackObj->flags & Flags22::unk_00)
                        {
                            if (frontBogie->isOnRackRail())
                            {
                                // 4a9d08 continue
                            }
                        }
                    }
                    else
                    {
                        if (frontBogie->trackType == 0xFF)
                        {
                            // 4a9d08 continue
                        }
                        const auto* roadObj = ObjectManager::get<RoadObject>(frontBogie->trackType);
                        if (roadObj->flags & Flags12::unk_04)
                        {
                            // 4a9d08 continue
                        }

                        if (roadObj->flags & Flags12::unk_05)
                        {
                            if (frontBogie->isOnRackRail())
                            {
                                // 4a9d08 continue
                            }
                        }
                    }
                    for (auto& carComponent : car)
                    {
                        carComponent.front->var_5E = 1;
                        carComponent.back->var_5E = 1;
                        carComponent.body->var_5E = 1;
                    }
                }
            }
            // 4a9d08
            if (frontBogie->var_5E != 0)
            {
                dh = 1;
            }
            if (_500170[enumValue(sprite_pitch)] <= -19182)
            {
                if (vehObject->power != 0)
                {
                    isOnRackRail = frontBogie->isOnRackRail();
                }
            }
            ebp += (frontBogie->var_5E * _500170[enumValue(sprite_pitch)]) / 256;
        }
        if (!isOnRackRail)
        {
            ebp /= 2;
            if (!(train.head->var_0C & Flags0C::unk_0))
            {
                var_5A = 4;
                if (currentSpeed <= 3.0_mph && train.head->owner == CompanyManager::getControllingId())
                {
                    MessageManager::post(MessageType::vehicleSlipped, train.head->owner, enumValue(train.head->id), 0xFFFF);
                }
            }
        }
        if (isOnRackRail && dh == 0)
        {
            if (train.head->var_0C & Flags0C::manualControl)
            {
                const auto manualSpeed = train.head->var_6E;
                if (manualSpeed < 0)
                {
                    if (manualSpeed <= -10)
                    {
                        var_5A = 3;
                    }
                    else
                    {
                        var_5A = 2;
                    }
                }
                else
                {
                    if (manualSpeed >= 10)
                    {
                        var_5A = 1;
                    }
                    else
                    {
                        var_5A = 2;
                    }
                }
                const auto power = (var_73 & (1 << 0)) ? totalPower / 4 : totalPower;
                ebp += ((power / 2048) * std::abs(manualSpeed)) / (totalWeight * 40);
            }
            else
            {
                const auto power = (var_73 & (1 << 0)) ? totalPower / 4 : totalPower;
                ebp += (power / 2048) / totalWeight;
            }
        }
        const auto speedSquare = toSpeed16(currentSpeed).getRaw() * toSpeed16(currentSpeed).getRaw();
        ebp -= speedSquare;
        const auto speedAdjustment = std::min(Speed32(ebp), 0.5_mph);
        auto newSpeed = speedAdjustment + currentSpeed;
        if (speedAdjustment < 0.0_mph)
        {
            auto minSpeed = 5.0_mph;
            if (train.head->var_0C & Flags0C::manualControl || !isOnRackRail)
            {
                minSpeed = 0.0_mph;
            }
            if (currentSpeed >= minSpeed)
            {
                newSpeed = std::max(newSpeed, minSpeed);
            }
        }

        if (!(train.head->var_0C & Flags0C::manualControl))
        {
            newSpeed = std::min(newSpeed, *vehicleUpdate_var_1136134);
        }
        currentSpeed = newSpeed;

        // 4A9F20

        vehicleUpdate_var_1136114 = (1 << 15);
        vehicleUpdate_var_113612C = sub_4B15FF(vehicleUpdate_var_113612C);
        vehicleUpdate_var_1136130 = vehicleUpdate_var_113612C;
        if (vehicleUpdate_var_1136114 & (1 << 1))
        {
            sub_4AA464();
            return false;
        }

        if (vehicleUpdate_var_1136114 & (1 << 0))
        {
            if (!(train.head->var_0C & Flags0C::manualControl))
            {
                currentSpeed = 0.0_mph;
                var_5A = 0;
            }
        }

        if (var_5A == 4)
        {
            vehicleUpdate_var_1136130 = vehicleUpdate_var_113612C + 0x1388;
        }

        train.head->var_3C -= vehicleUpdate_var_113612C;
        train.veh1->var_3C -= vehicleUpdate_var_113612C;

        if (var_5A == 3)
        {
            if (var_5B == 0)
            {
                invalidateSprite();
                train.veh1->invalidateSprite();
                train.tail->invalidateSprite();
                for (auto& car : train.cars)
                {
                    for (auto& carComponent : car)
                    {
                        carComponent.front->invalidateSprite();
                        carComponent.back->invalidateSprite();
                        carComponent.body->invalidateSprite();
                    }
                }
            }

            var_5B = 7;
            return true;
        }
        else
        {
            if (var_5B == 0)
            {
                return true;
            }
            var_5B--;
            if (var_5B == 0)
            {
                invalidateSprite();
                train.veh1->invalidateSprite();
                train.tail->invalidateSprite();
                for (auto& car : train.cars)
                {
                    for (auto& carComponent : car)
                    {
                        carComponent.front->invalidateSprite();
                        carComponent.back->invalidateSprite();
                        carComponent.body->invalidateSprite();
                    }
                }
            }
            return true;
        }
    }
}
