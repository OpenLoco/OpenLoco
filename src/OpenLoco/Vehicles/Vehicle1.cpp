#include "../Interop/Interop.hpp"
#include "../Objects/BridgeObject.h"
#include "../Objects/RoadObject.h"
#include "Vehicle.h"
#include "VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<Speed32, 0x01136134> vehicleUpdate_var_1136134; // Speed
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;

    // 0x004A9788
    bool Vehicle1::update()
    {
        switch (mode)
        {
            case TransportMode::air:
            case TransportMode::water:
                return true;
            case TransportMode::road:
                return updateRoad();
            case TransportMode::rail:
                return updateRail();
            default:
                return false;
        }
    }

    // 0x004F72FC
    std::array<uint16_t, 10> _roadIdToCurveSpeedFraction = {
        0xFFFF, // 1.00
        0x0CCD, // 0.05
        0x0CCD, // 0.05
        0x199A, // 0.10
        0x199A, // 0.10
        0xFFFF, // 1.00
        0xFFFF, // 1.00
        0xFFFF, // 1.00
        0xFFFF, // 1.00
        0x0CCD, // 0.05
    };

    // 0x004A9969
    bool Vehicle1::updateRoad()
    {
        uint16_t curveSpeedFraction = std::numeric_limits<uint16_t>::max();
        Speed16 maxSpeed = kSpeed16Max;
        RoutingManager::RingView ring(routingHandle);
        bool isOnRackRail = false;
        for (auto iter = ring.rbegin(); iter != ring.rend(); ++iter)
        {
            auto res = RoutingManager::getRouting(*iter);
            isOnRackRail |= (res & (1 << 13)) != 0; // rackrail
            uint8_t roadId = (res >> 3) & 0xF;
            curveSpeedFraction = std::min(curveSpeedFraction, _roadIdToCurveSpeedFraction[roadId]);
            if (res & (1 << 12))
            {
                const auto* bridgeObj = ObjectManager::get<BridgeObject>((res & 0xE00) >> 9);
                if (bridgeObj->max_speed != kSpeed16Null)
                {
                    maxSpeed = std::min(bridgeObj->max_speed, maxSpeed);
                }
            }
        }

        Vehicle train(head);
        if (train.veh2->var_4F != -1)
        {
            const auto* roadObj = ObjectManager::get<RoadObject>(train.veh2->var_4F);
            const Speed32 fractionalSpeed = Speed32(static_cast<uint32_t>(curveSpeedFraction) * roadObj->maxSpeed.getRaw());
            maxSpeed = std::min(toSpeed16(fractionalSpeed + 1.0_mph), maxSpeed);
            maxSpeed = std::max(maxSpeed, 12_mph);

            if (train.head->var_38 & Flags38::unk_5)
            {
                maxSpeed += maxSpeed / 4;
                maxSpeed = std::min(roadObj->maxSpeed, maxSpeed);
            }
        }
        else
        {
            const Speed32 fractionalSpeed = Speed32(static_cast<uint32_t>(curveSpeedFraction) * (60_mph).getRaw());
            maxSpeed = toSpeed16(fractionalSpeed);
        }

        maxSpeed = std::min(maxSpeed, train.veh2->maxSpeed);
        if (isOnRackRail)
        {
            maxSpeed = std::min(maxSpeed, train.veh2->rackRailMaxSpeed);
        }

        // Distance to speed32 would be var_3C * 2 but since maxspeed is interms of speed16 we have this awkward 32768
        maxSpeed = std::min(maxSpeed, Speed16(var_3C / 32768) + 5_mph);

        if ((train.head->var_0C & Flags0C::manualControl && train.head->var_6E <= -20)
            || (train.head->var_0C & Flags0C::commandStop))
        {
            if (train.veh2->currentSpeed == 0.0_mph)
            {
                maxSpeed = 0_mph;
            }
        }
        var_44 = maxSpeed;

        vehicleUpdate_var_1136134 = maxSpeed;
        int32_t distance1 = (train.veh2->currentSpeed / 2).getRaw() - var_3C;
        const auto unk2 = std::max(vehicleUpdate_var_113612C * 4, 0xCC48);

        distance1 = std::min(distance1, unk2);
        var_3C += distance1 - updateRoadMotion(distance1);

        if (!(vehicleUpdate_var_1136114 & (1 << 1)))
        {
            return true;
        }

        train.head->sub_4AD93A();
        if (train.head->status == Status::approaching)
        {
            train.head->status = Status::travelling;
        }
        return true;
    }

    // 0x004A97A6
    bool Vehicle1::updateRail()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A97A6, regs) & X86_FLAG_CARRY);
    }

    // 0x0047CA71
    int32_t Vehicle1::updateRoadMotion(const int32_t distance)
    {
        registers regs;
        regs.esi = X86Pointer(this);
        regs.eax = distance;

        call(0x0047CA71, regs);
        return regs.eax;
    }
}
