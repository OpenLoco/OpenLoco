#include "../Interop/Interop.hpp"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Tutorial.h"
#include "Vehicle.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<Speed32, 0x01136134> vehicleUpdate_var_1136134; // Speed
    static loco_global<VehicleHead*, 0x01136118> vehicleUpdate_head;

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
            // 0x004A9EF0
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

        uint8_t dl = 0;
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
                            // 4A8D08 continue
                        }
                        if (trackObj->flags & Flags22::unk_00)
                        {
                            //res = sub_4AA97A
                            if (res == 0)
                            {
                                // 4A8D08 continue
                            }
                        }
                    }
                    else
                    {
                        if (frontBogie->trackType == 0xFF)
                        {
                            // 4A8D08 continue
                        }
                        const auto* roadObj = ObjectManager::get<RoadObject>(frontBogie->trackType);
                        if (roadObj->flags & Flags12::unk_04)
                        {
                            // 4A8D08 continue
                        }

                        if (roadObj->flags & Flags12::unk_05)
                        {
                            //res = sub_4AA97A
                            if (res == 0)
                            {
                                // 4A8D08 continue
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
        }
        // 4A9DA1
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A9B0B, regs) & X86_FLAG_CARRY);
    }
}
