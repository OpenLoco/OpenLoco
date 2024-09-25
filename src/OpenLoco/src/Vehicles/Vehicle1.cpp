#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/BridgeObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "RoutingManager.h"
#include "Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> _vehicleUpdate_var_113612C; // Speed
    static loco_global<Speed32, 0x01136134> _vehicleUpdate_var_1136134; // Speed
    static loco_global<uint32_t, 0x01136114> _vehicleUpdate_var_1136114;

    // If distance travelled in one tick this is the speed
    constexpr Speed32 speedFromDistanceInATick(int32_t distance)
    {
        return Speed32(distance * 2);
    }

    constexpr int32_t distanceTraveledInATick(Speed32 speed)
    {
        return (speed / 2).getRaw();
    }
    static_assert(distanceTraveledInATick(speedFromDistanceInATick(100)) == 100);

    // 0x004A9788
    bool Vehicle1::update(bool honk)
    {
        switch (mode)
        {
            case TransportMode::air:
            case TransportMode::water:
                return true;
            case TransportMode::road:
                return updateRoad();
            case TransportMode::rail:
                return updateRail(honk);
            default:
                return false;
        }
    }

    // 0x004A9969
    bool Vehicle1::updateRoad()
    {
        uint16_t curveSpeedFraction = std::numeric_limits<uint16_t>::max();
        Speed16 newTargetSpeed = kSpeed16Max;
        RoutingManager::RingView ring(routingHandle);
        bool isOnRackRail = false;
        for (auto iter = ring.rbegin(); iter != ring.rend(); ++iter)
        {
            auto res = RoutingManager::getRouting(*iter);
            isOnRackRail |= (res & (1 << 13)) != 0; // rackrail
            uint8_t roadId = (res >> 3) & 0xF;
            curveSpeedFraction = std::min(curveSpeedFraction, World::TrackData::getRoadMiscData(roadId).curveSpeedFraction);
            if (res & (1 << 12))
            {
                const auto* bridgeObj = ObjectManager::get<BridgeObject>((res & 0xE00) >> 9);
                if (bridgeObj->maxSpeed != kSpeed16Null)
                {
                    newTargetSpeed = std::min(bridgeObj->maxSpeed, newTargetSpeed);
                }
            }
        }

        Vehicle train(head);
        if (train.veh2->var_4F != -1)
        {
            const auto* roadObj = ObjectManager::get<RoadObject>(train.veh2->var_4F);
            const Speed32 fractionalSpeed = Speed32(static_cast<uint32_t>(curveSpeedFraction) * roadObj->maxSpeed.getRaw());
            newTargetSpeed = std::min(toSpeed16(fractionalSpeed + 1.0_mph), newTargetSpeed);
            newTargetSpeed = std::max(newTargetSpeed, 12_mph);

            if (train.head->has38Flags(Flags38::unk_5))
            {
                newTargetSpeed += newTargetSpeed / 4;
                newTargetSpeed = std::min(roadObj->maxSpeed, newTargetSpeed);
            }
        }
        else
        {
            const Speed32 fractionalSpeed = Speed32(static_cast<uint32_t>(curveSpeedFraction) * (60_mph).getRaw());
            newTargetSpeed = toSpeed16(fractionalSpeed);
        }

        newTargetSpeed = std::min(newTargetSpeed, train.veh2->maxSpeed);
        if (isOnRackRail)
        {
            newTargetSpeed = std::min(newTargetSpeed, train.veh2->rackRailMaxSpeed);
        }

        // TODO: Original CS Bug. Fix when we diverge on replays
        // newTargetSpeed = std::min(newTargetSpeed, toSpeed16(speedFromDistanceInATick(var_3C)) + 5_mph);
        newTargetSpeed = std::min(newTargetSpeed, Speed16(static_cast<uint32_t>(var_3C) >> 15) + 5_mph);

        if ((train.head->hasVehicleFlags(VehicleFlags::manualControl) && train.head->manualPower <= -20)
            || train.head->hasVehicleFlags(VehicleFlags::commandStop))
        {
            if (train.veh2->currentSpeed == 0.0_mph)
            {
                newTargetSpeed = 0_mph;
            }
        }
        targetSpeed = newTargetSpeed;

        _vehicleUpdate_var_1136134 = newTargetSpeed;
        int32_t distance1 = distanceTraveledInATick(train.veh2->currentSpeed) - var_3C;
        const auto unk2 = std::max(_vehicleUpdate_var_113612C * 4, 0xCC48);

        distance1 = std::min(distance1, unk2);
        var_3C += distance1 - updateRoadMotion(distance1);

        if (!(_vehicleUpdate_var_1136114 & (1 << 1)))
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

    // 0x004B98DA
    static void railProduceCrossingWhistle(const Vehicle2& veh2)
    {
        Vehicle train{ veh2.head };
        auto* vehObj = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->objectId);

        if (vehObj->numStartSounds == 0)
        {
            return;
        }

        gPrng1().randNext(); // TODO: Remove when we can diverge from vanilla

        const auto soundNum = (vehObj->numStartSounds & NumStartSounds::kMask) - 1;
        const auto soundObjId = vehObj->startSounds[soundNum];

        const auto height = World::TileManager::getHeight(veh2.position);
        const auto volume = veh2.position.z < height.landHeight ? -1500 : 0;

        Audio::playSound(Audio::makeObjectSoundId(soundObjId), veh2.position + World::Pos3{ 0, 0, 22 }, volume, 22050);
    }

    // 0x004A97A6
    bool Vehicle1::updateRail(bool honk)
    {
        Vehicle train{ head };
        uint16_t curveSpeedFraction = std::numeric_limits<uint16_t>::max();
        Speed16 newTargetSpeed = kSpeed16Max;
        bool isOnRackRail = false;
        if (!train.head->hasVehicleFlags(VehicleFlags::manualControl))
        {
            RoutingManager::RingView ring(routingHandle);
            for (auto iter = ring.rbegin(); iter != ring.rend(); ++iter)
            {
                auto res = RoutingManager::getRouting(*iter);
                isOnRackRail |= (res & (1U << 13)) != 0; // rackrail
                uint8_t trackId = (res >> 3) & 0x3F;
                curveSpeedFraction = std::min(curveSpeedFraction, World::TrackData::getTrackMiscData(trackId).curveSpeedFraction);
                if (res & (1U << 12))
                {
                    const auto* bridgeObj = ObjectManager::get<BridgeObject>((res & 0xE00) >> 9);
                    if (bridgeObj->maxSpeed != kSpeed16Null)
                    {
                        newTargetSpeed = std::min(bridgeObj->maxSpeed, newTargetSpeed);
                    }
                }
            }
        }

        const auto* trackObj = ObjectManager::get<TrackObject>(trackType);
        const Speed32 fractionalSpeed = Speed32(static_cast<uint32_t>(curveSpeedFraction) * trackObj->curveSpeed.getRaw());
        newTargetSpeed = std::min(toSpeed16(fractionalSpeed + 1.0_mph), newTargetSpeed);

        if (train.head->has38Flags(Flags38::unk_5))
        {
            newTargetSpeed += newTargetSpeed / 4;
            newTargetSpeed = std::min(trackObj->curveSpeed, newTargetSpeed);
        }

        const auto veh2MaxSpeed = [veh2 = train.veh2]() {
            if (veh2->has73Flags(Flags73::isBrokenDown))
            {
                return veh2->maxSpeed / 4;
            }
            return veh2->maxSpeed;
        }();

        newTargetSpeed = std::min(newTargetSpeed, veh2MaxSpeed);
        if (isOnRackRail)
        {
            newTargetSpeed = std::min(newTargetSpeed, train.veh2->rackRailMaxSpeed);
        }

        if (!train.head->hasVehicleFlags(VehicleFlags::manualControl))
        {
            // TODO: Original CS Bug. Fix when we diverge on replays
            // newTargetSpeed = std::min(newTargetSpeed, toSpeed16(speedFromDistanceInATick(var_3C)) + 5_mph);
            newTargetSpeed = std::min(newTargetSpeed, Speed16(static_cast<uint32_t>(var_3C) >> 15) + 5_mph);
        }

        if ((train.head->hasVehicleFlags(VehicleFlags::manualControl) && train.head->manualPower <= -20)
            || train.head->hasVehicleFlags(VehicleFlags::commandStop))
        {
            if (train.veh2->currentSpeed == 0.0_mph)
            {
                newTargetSpeed = 0_mph;
            }
        }
        targetSpeed = newTargetSpeed;

        _vehicleUpdate_var_1136134 = newTargetSpeed;
        int32_t distance1 = distanceTraveledInATick(train.veh2->currentSpeed) - var_3C;
        const auto unk2 = std::max(_vehicleUpdate_var_113612C * 4, 0xCC48);

        distance1 = std::min(distance1, unk2);
        _vehicleUpdate_var_1136114 = 0;
        var_3C += distance1 - updateTrackMotion(distance1);

        if (honk)
        {
            railProduceCrossingWhistle(*train.veh2);
            return true;
        }

        if (!(_vehicleUpdate_var_1136114 & (1U << 1)))
        {
            if (_vehicleUpdate_var_1136114 & (1U << 4))
            {
                railProduceCrossingWhistle(*train.veh2);
            }
            return true;
        }

        train.head->sub_4AD93A();
        if (train.head->status == Status::approaching)
        {
            train.head->status = Status::travelling;
        }

        return true;
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
