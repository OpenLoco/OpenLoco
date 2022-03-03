#include "../Interop/Interop.hpp"
#include "../Objects/BridgeObject.h"
#include "../Objects/RoadObject.h"
#include "Vehicle.h"
#include "VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
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
        Speed16 maxSpeed = 32767_mph;
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
                auto* bridgeObj = ObjectManager::get<BridgeObject>((res & 0xE00) >> 9);
                maxSpeed = std::min(bridgeObj->max_speed, maxSpeed);
            }
        }

        Vehicle train(head);
        if (train.veh2->var_4F != -1)
        {
            auto* roadObj = ObjectManager::get<RoadObject>(train.veh2->var_4F);
            maxSpeed = std::min(toSpeed16(Speed32(curveSpeedFraction * roadObj->maxSpeed.getRaw()) + 1.0_mph), maxSpeed);
            maxSpeed = std::max(maxSpeed, 12_mph);
            if (train.head->var_38 & Flags38::unk_5)
            {
                maxSpeed += maxSpeed / 4;
                maxSpeed = std::min(roadObj->maxSpeed, maxSpeed);
            }
        }
        else
        {
            maxSpeed = toSpeed16(Speed32((60_mph * curveSpeedFraction).getRaw()));
        }

        maxSpeed = std::min(maxSpeed, train.veh2->maxSpeed);
        if (isOnRackRail)
        {
            maxSpeed = std::min(maxSpeed, train.veh2->rackRailMaxSpeed);
        }
        // 0x4A9A56

        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A9969, regs) & X86_FLAG_CARRY);
    }

    // 0x004A97A6
    bool Vehicle1::updateRail()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A97A6, regs) & X86_FLAG_CARRY);
    }
}
