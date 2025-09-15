#include "Entities/EntityManager.h"
#include "Map/TileManager.h"
#include "Map/Track/SubpositionData.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Objects/BridgeObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "RoutingManager.h"
#include "Vehicle.h"
#include "ViewportManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> _vehicleUpdate_var_113612C; // Speed
    static loco_global<Speed32, 0x01136134> _vehicleUpdate_var_1136134; // Speed

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

            if (train.head->has38Flags(Flags38::fasterAroundCurves))
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

        if (!hasUpdateVar1136114Flags(UpdateVar1136114Flags::noRouteFound))
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
    bool Vehicle1::updateRail()
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

        if (train.head->has38Flags(Flags38::fasterAroundCurves))
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
        resetUpdateVar1136114Flags();
        var_3C += distance1 - updateTrackMotion(distance1);

        if (!hasUpdateVar1136114Flags(UpdateVar1136114Flags::noRouteFound))
        {
            if (hasUpdateVar1136114Flags(UpdateVar1136114Flags::approachingGradeCrossing))
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

    enum class OvertakeResult
    {
        overtakeAvailble,
        noOvertakeAvailble,
        mayBeOvertaken,
    };

    // 0x0047CD78
    // ax : pos.x
    // cx : pos.y
    // dl : pos.z / World::kSmallZStep
    // ebp : tad
    // esi : veh1
    //
    // return mayBeOvertaken == high carry flag
    //        overtakeAvailable   0x0112C328 == 5
    //        noOvertakeAvailable 0x0112C328 == 0
    static OvertakeResult getRoadOvertakeAvailability(const Vehicle1& veh1, World::Pos3 pos, uint16_t tad)
    {
        OvertakeResult result = OvertakeResult::noOvertakeAvailble;
        for (const auto& nearby : kMooreNeighbourhood)
        {
            const auto inspectionPos = World::toTileSpace(pos) + nearby;
            for (auto* entity : EntityManager::EntityTileList(World::toWorldSpace(inspectionPos)))
            {
                auto* vehicleBase = entity->asBase<VehicleBase>();
                if (vehicleBase == nullptr || !vehicleBase->isVehicleTail())
                {
                    continue;
                }
                auto* vehicleTail = vehicleBase->asVehicleTail();
                if (vehicleTail == nullptr)
                {
                    continue;
                }
                if (vehicleTail->getTransportMode() != TransportMode::road)
                {
                    continue;
                }
                if (vehicleTail->tileBaseZ * World::kSmallZStep != pos.z)
                {
                    continue;
                }
                if (vehicleTail->tileX != pos.x || vehicleTail->tileY != pos.y)
                {
                    continue;
                }
                if ((vehicleTail->trackAndDirection.road._data & World::Track::AdditionalTaDFlags::basicTaDMask) != (tad & World::Track::AdditionalTaDFlags::basicTaDMask))
                {
                    continue;
                }

                // Perhaps a little expensive to do this (we don't do it on rail vehicles for example)
                Vehicle train(vehicleTail->head);
                if (train.veh1->var_3C < 0x220C0)
                {
                    continue;
                }
                if ((train.veh2->var_73 & Flags73::isBrokenDown) != Flags73::none)
                {
                    continue;
                }
                if (veh1.var_3C < train.veh1->var_3C)
                {
                    return OvertakeResult::mayBeOvertaken;
                }
                auto* veh2 = EntityManager::get<Vehicle2>(veh1.nextCarId);
                if (veh2 == nullptr)
                {
                    continue;
                }
                if (veh2->maxSpeed <= train.veh2->maxSpeed)
                {
                    return OvertakeResult::mayBeOvertaken;
                }
                result = OvertakeResult::overtakeAvailble;
            }
        }
        return result;
    }

    // 0x0047D308
    static bool veh1UpdateRoadMotionNewRoadPiece(Vehicle1& veh1)
    {
        auto newRoutingHandle = veh1.routingHandle;
        auto newIndex = newRoutingHandle.getIndex() + 1;
        newRoutingHandle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(newRoutingHandle);
        if (routing == RoutingManager::kAllocatedButFreeRoutingStation)
        {
            return false;
        }

        const auto newPos = World::Pos3(veh1.tileX, veh1.tileY, veh1.tileBaseZ * World::kSmallZStep)
            + World::TrackData::getUnkRoad(veh1.trackAndDirection.road._data & 0x7F).pos;

        TrackAndDirection::_RoadAndDirection newRad(0, 0);
        newRad._data = routing & 0x1FFU;
        veh1.sub_47D959(newPos, newRad, true);

        veh1.routingHandle = newRoutingHandle;
        veh1.trackAndDirection.road = newRad;

        veh1.tileX = newPos.x;
        veh1.tileY = newPos.y;
        veh1.tileBaseZ = newPos.z / World::kSmallZStep;
        return true;
    }

    // 0x0047D2D6
    // veh1 : esi
    // numRoadPieces : ah
    // return eax
    static int32_t vehicle1UpdateRoadMotionByPieces(Vehicle1& veh1, const uint8_t numRoadPieces)
    {
        auto intermediatePosition = veh1.position;
        auto distanceMoved = 0;
        for (auto i = 0; i < numRoadPieces;)
        {
            auto newSubPosition = veh1.subPosition + 1U;
            const auto subPositionDataSize = World::TrackData::getRoadSubPositon(veh1.trackAndDirection.road._data).size();
            // This means we have moved forward by a road piece
            if (newSubPosition >= subPositionDataSize)
            {
                if (!veh1UpdateRoadMotionNewRoadPiece(veh1))
                {
                    // This seems wrong but its what vanilla does
                    // Todo investigate if we should return 0 or actually move
                    // the vehicle
                    return distanceMoved;
                }
                else
                {
                    newSubPosition = 0;
                    i++;
                }
            }
            veh1.subPosition = newSubPosition;
            const auto& moveData = World::TrackData::getRoadSubPositon(veh1.trackAndDirection.road._data)[newSubPosition];
            const auto nextNewPosition = moveData.loc + World::Pos3(veh1.tileX, veh1.tileY, veh1.tileBaseZ * World::kSmallZStep);
            distanceMoved += kMovementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
            intermediatePosition = nextNewPosition;
            veh1.spriteYaw = moveData.yaw;
            veh1.spritePitch = moveData.pitch;
        }

        veh1.moveTo(intermediatePosition);
        return distanceMoved;
    }

    // 0x0047D1B4
    // Similar to the others but doesn't update the vehicle
    // due to that it must take reference to all the variables
    static bool veh1UpdateRoadMotionNewRoadPieceNoMove(World::Pos3& pos, RoutingHandle& handle, TrackAndDirection::_RoadAndDirection& rad)
    {
        auto newIndex = handle.getIndex() + 1;
        handle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(handle);
        if (routing == RoutingManager::kAllocatedButFreeRoutingStation)
        {
            return false;
        }

        pos += World::TrackData::getUnkRoad(rad._data & 0x7F).pos;

        rad._data = routing & 0x1FFU;
        return true;
    }

    // 0x0047D142
    // veh1 : esi
    // numRoadPieces : ah
    // return eax
    static int32_t vehicle1UpdateRoadMotionByPiecesNoMove(Vehicle1& veh1, const uint8_t numRoadPieces)
    {
        auto intermediatePosition = veh1.position;
        auto distanceMoved = 0;
        auto pos = World::Pos3(veh1.tileX, veh1.tileY, veh1.tileBaseZ * World::kSmallZStep);
        auto rad = veh1.trackAndDirection.road;
        auto handle = veh1.routingHandle;
        auto subPosition = veh1.subPosition;
        for (auto i = 0; i < numRoadPieces;)
        {
            auto newSubPosition = subPosition + 1U;
            const auto subPositionDataSize = World::TrackData::getRoadSubPositon(rad._data).size();
            // This means we have moved forward by a road piece
            if (newSubPosition >= subPositionDataSize)
            {
                if (!veh1UpdateRoadMotionNewRoadPieceNoMove(pos, handle, rad))
                {
                    return distanceMoved;
                }
                else
                {
                    newSubPosition = 0;
                    i++;
                }
            }
            subPosition = newSubPosition;
            const auto& moveData = World::TrackData::getRoadSubPositon(rad._data)[newSubPosition];
            const auto nextNewPosition = moveData.loc + pos;
            distanceMoved += kMovementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
            intermediatePosition = nextNewPosition;
        }
        return distanceMoved;
    }

    // 0x0047D46F
    // veh1 : esi
    // numRoadPieces : ah
    static void applyOvertakeToVehicle1(Vehicle1& veh1, uint8_t numRoadPieces)
    {
        const auto noOvertakeDistance = vehicle1UpdateRoadMotionByPiecesNoMove(veh1, numRoadPieces);
        auto routingRing = RoutingManager::RingView(veh1.routingHandle);
        auto iter = ++routingRing.begin();
        constexpr auto kResetRouting = static_cast<uint16_t>(~((1U << 7) | (1U << 8)));
        for (auto i = 0; i < numRoadPieces; ++iter, ++i)
        {
            if (iter == routingRing.end())
            {
                break;
            }
            const auto routing = RoutingManager::getRouting(*iter);
            if (i == 0)
            {
                RoutingManager::setRouting(*iter, (routing & kResetRouting) | (1U << 8));
            }
            else if (i == (numRoadPieces - 1))
            {
                RoutingManager::setRouting(*iter, (routing & kResetRouting) | (1U << 8) | (1U << 7));
            }
            else
            {
                RoutingManager::setRouting(*iter, (routing & kResetRouting) | (1 << 7));
            }
        }

        const auto newDistance = vehicle1UpdateRoadMotionByPieces(veh1, numRoadPieces);
        veh1.var_3C += newDistance;
        auto* head = EntityManager::get<VehicleHead>(veh1.head);
        head->var_3C += newDistance - noOvertakeDistance;
    }

    // 0x0047D52B
    // veh1 : esi
    // numRoadPieces : ah
    static void applyChangeLaneToVehicle1(Vehicle1& veh1, uint8_t numRoadPieces)
    {
        const auto noOvertakeDistance = vehicle1UpdateRoadMotionByPiecesNoMove(veh1, numRoadPieces);
        auto routingRing = RoutingManager::RingView(veh1.routingHandle);
        auto iter = ++routingRing.begin();
        constexpr auto kResetRouting = static_cast<uint16_t>(~((1U << 7) | (1U << 8)));
        for (auto i = 0; i < numRoadPieces; ++iter, ++i)
        {
            if (iter == routingRing.end())
            {
                break;
            }
            const auto routing = RoutingManager::getRouting(*iter);
            if (i == 0)
            {
                RoutingManager::setRouting(*iter, (routing & kResetRouting) | (1U << 8));
            }
            else
            {
                RoutingManager::setRouting(*iter, (routing & kResetRouting) | (1 << 7));
            }
        }

        const auto newDistance = vehicle1UpdateRoadMotionByPieces(veh1, numRoadPieces);
        veh1.var_3C += newDistance;
        auto* head = EntityManager::get<VehicleHead>(veh1.head);
        head->var_3C += newDistance - noOvertakeDistance;
        head->trackAndDirection.road._data &= kResetRouting;
        head->trackAndDirection.road._data |= numRoadPieces <= 1 ? (1U << 8) : (1U << 7);
    }
    struct Sub47CEB7Result
    {
        uint8_t al;
        uint8_t ah;
    };

    // 0x0047CEB7
    // veh1 : esi
    // return : al, ah
    static Sub47CEB7Result sub_47CEB7(Vehicle1& veh1)
    {
        const auto startPos = World::Pos3(veh1.tileX, veh1.tileY, veh1.tileBaseZ * World::kSmallZStep);
        uint8_t unk112C327 = 0;

        auto pos = startPos + World::TrackData::getUnkRoad(veh1.trackAndDirection.road._data & 0x7F).pos;
        unk112C327++;

        auto routings = RoutingManager::RingView(veh1.routingHandle);
        auto routingIter = routings.begin();
        if (routingIter == routings.end())
        {
            return Sub47CEB7Result{ 0, 0 };
        }
        routingIter++;
        if (routingIter == routings.end())
        {
            return Sub47CEB7Result{ 0, 0 };
        }
        auto routing = RoutingManager::getRouting(*routingIter);
        TrackAndDirection::_RoadAndDirection tad(0, 0);
        tad._data = routing & World::Track::AdditionalTaDFlags::basicTaDMask;
        const auto occupationFlags = getRoadOccupation(pos, tad);

        if ((occupationFlags & RoadOccupationFlags::hasLevelCrossing) != RoadOccupationFlags::none)
        {
            // 0x0047D0B9
            if ((occupationFlags & (RoadOccupationFlags::isLaneOccupied | RoadOccupationFlags::isLevelCrossingClosed)) != RoadOccupationFlags::none)
            {
                return Sub47CEB7Result{ 2, 0 };
            }
            ++unk112C327;
            for (; unk112C327 < 255; ++unk112C327)
            {
                pos += World::TrackData::getUnkRoad(tad._data & 0x7F).pos;
                routingIter++;
                if (routingIter == routings.end())
                {
                    return Sub47CEB7Result{ 2, 0 };
                }
                tad._data = RoutingManager::getRouting(*routingIter) & World::Track::AdditionalTaDFlags::basicTaDMask;

                const auto fwdOccupationFlags = getRoadOccupation(pos, tad);
                if ((fwdOccupationFlags & (RoadOccupationFlags::isLaneOccupied | RoadOccupationFlags::isLevelCrossingClosed)) != RoadOccupationFlags::none)
                {
                    return Sub47CEB7Result{ 2, 0 };
                }
                if ((fwdOccupationFlags & RoadOccupationFlags::hasLevelCrossing) == RoadOccupationFlags::none)
                {
                    return Sub47CEB7Result{ 4, unk112C327 };
                }
            }
            return Sub47CEB7Result{ 2, 0 };
        }
        else
        {
            OvertakeResult overtakeResult = OvertakeResult::overtakeAvailble;
            if ((occupationFlags & RoadOccupationFlags::isLaneOccupied) != RoadOccupationFlags::none)
            {
                overtakeResult = getRoadOvertakeAvailability(veh1, pos, tad._data);
                if (overtakeResult == OvertakeResult::mayBeOvertaken)
                {
                    return Sub47CEB7Result{ 0, 0 };
                }
            }
            else
            {
                // 0x0047CF32
                auto nextRoutingIter = routingIter;
                nextRoutingIter++;
                const auto nextPos = pos + World::TrackData::getUnkRoad(tad._data & 0x7F).pos;
                if (nextRoutingIter == routings.end())
                {
                    return Sub47CEB7Result{ 0, 0 };
                }
                auto nextRouting = RoutingManager::getRouting(*nextRoutingIter);
                TrackAndDirection::_RoadAndDirection nextTad(0, 0);
                nextTad._data = nextRouting & World::Track::AdditionalTaDFlags::basicTaDMask;
                const auto nextOccupationFlags = getRoadOccupation(nextPos, nextTad);
                if ((nextOccupationFlags & RoadOccupationFlags::isLaneOccupied) == RoadOccupationFlags::none)
                {
                    return Sub47CEB7Result{ 0, 0 };
                }
                overtakeResult = getRoadOvertakeAvailability(veh1, nextPos, nextTad._data);
                if (overtakeResult == OvertakeResult::mayBeOvertaken)
                {
                    return Sub47CEB7Result{ 0, 0 };
                }
            }
            // 0x0047CFB5
            const auto& roadObj = ObjectManager::get<RoadObject>(veh1.trackType);
            if (!roadObj->hasFlags(RoadObjectFlags::isRoad))
            {
                return Sub47CEB7Result{ 0, 0 };
            }
            if (tad.isUnk8() || tad.isBackToFront())
            {
                return Sub47CEB7Result{ 0, 0 };
            }
            auto backToFrontTad = tad;
            backToFrontTad._data ^= (1U << 7);
            auto btfOccupationFlags = getRoadOccupation(pos, backToFrontTad);
            if ((btfOccupationFlags & (RoadOccupationFlags::hasLevelCrossing | RoadOccupationFlags::isLaneOccupied)) != RoadOccupationFlags::none)
            {
                return Sub47CEB7Result{ 0, 0 };
            }
            const auto unkCount = overtakeResult == OvertakeResult::overtakeAvailble ? 5 : 0;
            ++unk112C327;
            auto i = 0;
            for (; unk112C327 < 11; ++unk112C327)
            {
                pos += World::TrackData::getUnkRoad(tad._data & 0x7F).pos;
                routingIter++;
                if (routingIter == routings.end())
                {
                    if ((btfOccupationFlags & RoadOccupationFlags::hasStation) == RoadOccupationFlags::none)
                    {
                        return Sub47CEB7Result{ 0, 0 };
                    }
                    return Sub47CEB7Result{ 8, static_cast<uint8_t>(unk112C327 - 1) };
                }
                tad._data = RoutingManager::getRouting(*routingIter) & World::Track::AdditionalTaDFlags::basicTaDMask;

                backToFrontTad = tad;
                backToFrontTad._data ^= (1U << 7);
                btfOccupationFlags = getRoadOccupation(pos, backToFrontTad);
                if ((btfOccupationFlags & (RoadOccupationFlags::hasLevelCrossing | RoadOccupationFlags::isLaneOccupied)) != RoadOccupationFlags::none)
                {
                    return Sub47CEB7Result{ 0, 0 };
                }
                const auto fwdOccupationFlags = getRoadOccupation(pos, tad);
                if ((fwdOccupationFlags & RoadOccupationFlags::isLaneOccupied) != RoadOccupationFlags::none)
                {
                    i = 0;
                    continue;
                }
                i++;
                if (i <= unkCount)
                {
                    continue;
                }
                return Sub47CEB7Result{ 1, unk112C327 };
            }
            return Sub47CEB7Result{ 0, 0 };
        }
    }

    enum class RoadMotionNewPieceResult
    {
        // New piece processed and any left over distance to process should be processed by parent function
        continueUpdate,

        // We have reached the end of the update processing for some reason e.g. invalid route parent function
        // should update the position and return left over distance
        noFurther,

        // When this happens parent function does not require any further processing and should return 0
        performedLookahead,
    };

    // 0x0047CABF
    static RoadMotionNewPieceResult updateRoadMotionNewRoadPiece(Vehicle1& component)
    {
        auto newRoutingHandle = component.routingHandle;
        auto newIndex = newRoutingHandle.getIndex() + 1;
        newRoutingHandle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(newRoutingHandle);
        if (routing == RoutingManager::kAllocatedButFreeRoutingStation)
        {
            return RoadMotionNewPieceResult::noFurther;
        }

        auto* head = EntityManager::get<VehicleHead>(component.head);
        if (head->var_52 != 1)
        {
            auto res = sub_47CEB7(component);
            if (res.al & (1U << 1))
            {
                return RoadMotionNewPieceResult::noFurther;
            }
            else if (res.al & (1U << 0))
            {
                applyOvertakeToVehicle1(component, res.ah);
                return RoadMotionNewPieceResult::performedLookahead;
            }
            else if (res.al & (1U << 3))
            {
                applyChangeLaneToVehicle1(component, res.ah);
                return RoadMotionNewPieceResult::performedLookahead;
            }
            else if (res.al & (1U << 2))
            {
                component.var_3C += vehicle1UpdateRoadMotionByPieces(component, res.ah);
                return RoadMotionNewPieceResult::performedLookahead;
            }
        }

        World::Pos3 pos(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);

        auto basicRad = component.trackAndDirection.road;
        basicRad._data = routing & 0x7F;
        auto [nextPos, nextRot] = World::Track::getRoadConnectionEnd(pos, basicRad._data);
        const auto tc = World::Track::getRoadConnections(nextPos, nextRot, component.owner, head->trackType, head->var_53, 0);

        bool routingFound = false;
        for (auto& connection : tc.connections)
        {
            if ((connection & 0x7F) == (routing & 0x7F))
            {
                routingFound = true;
                break;
            }
        }
        if (!routingFound)
        {
            setUpdateVar1136114Flags(UpdateVar1136114Flags::noRouteFound);
            return RoadMotionNewPieceResult::noFurther;
        }

        auto occupation = getRoadOccupation(nextPos, basicRad);
        const auto invalidOccupationFlags = RoadOccupationFlags::isLaneOccupied | (head->var_52 != 1 ? RoadOccupationFlags::isLevelCrossingClosed : RoadOccupationFlags::none);
        if ((occupation & invalidOccupationFlags) != RoadOccupationFlags::none)
        {
            return RoadMotionNewPieceResult::noFurther;
        }

        component.sub_47D959(nextPos, basicRad, true);

        component.routingHandle = newRoutingHandle;
        component.trackAndDirection.road._data = routing & 0x1FF;

        component.tileX = nextPos.x;
        component.tileY = nextPos.y;
        component.tileBaseZ = nextPos.z / World::kSmallZStep;
        return RoadMotionNewPieceResult::continueUpdate;
    }

    // 0x0047CA71
    int32_t Vehicle1::updateRoadMotion(const int32_t distance)
    {
        resetUpdateVar1136114Flags();

        this->remainingDistance += distance;
        bool hasMoved = false;
        auto returnValue = 0;
        auto intermediatePosition = this->position;
        while (this->remainingDistance >= 0x368A)
        {
            hasMoved = true;
            auto newSubPosition = this->subPosition + 1U;
            const auto subPositionDataSize = World::TrackData::getRoadSubPositon(this->trackAndDirection.road._data).size();
            // This means we have moved forward by a road piece
            if (newSubPosition >= subPositionDataSize)
            {
                auto newPieceRes = updateRoadMotionNewRoadPiece(*this);
                if (newPieceRes == RoadMotionNewPieceResult::noFurther)
                {
                    returnValue = this->remainingDistance - 0x3689;
                    this->remainingDistance = 0x3689;
                    setUpdateVar1136114Flags(UpdateVar1136114Flags::unk_m00);
                    break;
                }
                else if (newPieceRes == RoadMotionNewPieceResult::performedLookahead)
                {
                    return 0;
                }
                else
                {
                    newSubPosition = 0;
                }
            }
            // 0x0047C95B
            this->subPosition = newSubPosition;
            const auto& moveData = World::TrackData::getRoadSubPositon(this->trackAndDirection.road._data)[newSubPosition];
            const auto nextNewPosition = moveData.loc + World::Pos3(this->tileX, this->tileY, this->tileBaseZ * World::kSmallZStep);
            this->remainingDistance -= kMovementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
            intermediatePosition = nextNewPosition;
            this->spriteYaw = moveData.yaw;
            this->spritePitch = moveData.pitch;
        }
        if (hasMoved)
        {
            Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
            this->moveTo(intermediatePosition);
            Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
        }
        return returnValue;
    }
}
