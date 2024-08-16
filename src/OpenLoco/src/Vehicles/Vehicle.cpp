#include "Vehicle.h"
#include "Entities/EntityManager.h"
#include "GameState.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/SubpositionData.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Objects/AirportObject.h"
#include "Objects/ObjectManager.h"
#include "RoutingManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<uint8_t[128], 0x004F7358> _4F7358; // trackAndDirection without the direction 0x1FC
    static loco_global<uint32_t, 0x01136114> _vehicleUpdate_var_1136114;
    static loco_global<EntityId, 0x0113610E> _vehicleUpdate_collisionCarComponent;

#pragma pack(push, 1)
    // There are some common elements in the vehicle components at various offsets these can be accessed via VehicleBase
    struct VehicleCommon : VehicleBase
    {
        uint8_t pad_24[0x24 - 0x22];
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles
        Flags38 var_38;                      // 0x38
        uint8_t pad_39;
        EntityId nextCarId; // 0x3A
        uint8_t pad_3C[0x42 - 0x3C];
        TransportMode mode; // 0x42 field same in all vehicles
    };
    static_assert(sizeof(VehicleCommon) == 0x43); // Can't use offset_of change this to last field if more found
#pragma pack(pop)

    VehicleBase* VehicleBase::nextVehicle()
    {
        return EntityManager::get<VehicleBase>(nextEntityId);
    }

    VehicleBase* VehicleBase::nextVehicleComponent()
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        return EntityManager::get<VehicleBase>(veh->nextCarId);
    }

    TransportMode VehicleBase::getTransportMode() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->mode;
    }

    Flags38 VehicleBase::getFlags38() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->var_38;
    }

    uint8_t VehicleBase::getTrackType() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->trackType;
    }

    World::Pos3 VehicleBase::getTrackLoc() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return World::Pos3(veh->tileX, veh->tileY, veh->tileBaseZ * World::kSmallZStep);
    }

    TrackAndDirection VehicleBase::getTrackAndDirection() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->trackAndDirection;
    }

    RoutingHandle VehicleBase::getRoutingHandle() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->routingHandle;
    }

    EntityId VehicleBase::getHead() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->head;
    }

    int32_t VehicleBase::getRemainingDistance() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->remainingDistance;
    }

    void VehicleBase::setNextCar(const EntityId newNextCar)
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        veh->nextCarId = newNextCar;
    }

    bool VehicleBase::has38Flags(Flags38 flagsToTest) const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return (veh->var_38 & flagsToTest) != Flags38::none;
    }

    bool VehicleBase::hasVehicleFlags(VehicleFlags flagsToTest) const
    {
        const auto* ent = reinterpret_cast<const EntityBase*>(this);
        return (ent->vehicleFlags & flagsToTest) != VehicleFlags::none;
    }

    // 0x004AA464
    void VehicleBase::sub_4AA464()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004AA464, regs);
    }

    static bool updateRoadMotionNewRoadPiece(VehicleCommon& component)
    {
        auto newRoutingHandle = component.routingHandle;
        auto newIndex = newRoutingHandle.getIndex() + 1;
        newRoutingHandle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(newRoutingHandle);
        if (routing != RoutingManager::kAllocatedButFreeRoutingStation)
        {
            Vehicle train(component.head);
            if (_vehicleUpdate_var_1136114 & (1U << 15))
            {
                if (train.veh1->routingHandle == component.routingHandle)
                {
                    _vehicleUpdate_var_1136114 |= (1U << 3);
                    return false;
                }
            }
            World::Pos3 pos(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);

            auto [nextPos, nextRot] = World::Track::getRoadConnectionEnd(pos, component.trackAndDirection.road._data & 0x7F);
            const auto tc = World::Track::getRoadConnections(nextPos, nextRot, component.owner, component.trackType, train.head->var_53, 0);

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
                _vehicleUpdate_var_1136114 |= (1U << 1);
                return false;
            }
            component.routingHandle = newRoutingHandle;
            const auto oldTaD = component.trackAndDirection.road._data;
            component.trackAndDirection.road._data = routing & 0x1FF;
            if (component.isVehicle2())
            {
                component.asVehicle2()->var_4F = tc.roadObjectId;
            }

            pos += World::TrackData::getUnkRoad(oldTaD & 0x7F).pos;
            component.tileX = pos.x;
            component.tileY = pos.y;
            component.tileBaseZ = pos.z / World::kSmallZStep;
            return true;
        }

        return false;
    }

    static bool updateTrackMotionNewTrackPiece(VehicleCommon& component)
    {
        auto newRoutingHandle = component.routingHandle;
        auto newIndex = newRoutingHandle.getIndex() + 1;
        newRoutingHandle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(newRoutingHandle);
        if (routing != RoutingManager::kAllocatedButFreeRoutingStation)
        {
            Vehicle train(component.head);
            if (_vehicleUpdate_var_1136114 & (1U << 15))
            {
                if (train.veh1->routingHandle == component.routingHandle)
                {
                    _vehicleUpdate_var_1136114 |= (1U << 3);
                    return false;
                }
            }
            World::Pos3 pos(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);

            auto [nextPos, nextRot] = World::Track::getTrackConnectionEnd(pos, component.trackAndDirection.track._data);
            const auto tc = World::Track::getTrackConnections(nextPos, nextRot, component.owner, component.trackType, train.head->var_53, 0);
            if (tc.hasLevelCrossing)
            {
                _vehicleUpdate_var_1136114 |= (1U << 4);
            }
            bool routingFound = false;
            for (auto& connection : tc.connections)
            {
                if ((connection & 0x1FF) == (routing & 0x1FF))
                {
                    routingFound = true;
                    break;
                }
            }
            if (!routingFound)
            {
                _vehicleUpdate_var_1136114 |= (1U << 1);
                return false;
            }
            component.routingHandle = newRoutingHandle;
            const auto oldTaD = component.trackAndDirection.track._data;
            component.trackAndDirection.track._data = routing & 0x1FF;
            pos += World::TrackData::getUnkTrack(oldTaD).pos;
            component.tileX = pos.x;
            component.tileY = pos.y;
            component.tileBaseZ = pos.z / World::kSmallZStep;
            return true;
        }

        return false;
    }

    constexpr uint8_t getMovementNibble(const World::Pos3& pos1, const World::Pos3& pos2)
    {
        uint8_t nibble = 0;
        if (pos1.x != pos2.x)
        {
            nibble |= (1U << 0);
        }
        if (pos1.y != pos2.y)
        {
            nibble |= (1U << 1);
        }
        if (pos1.z != pos2.z)
        {
            nibble |= (1U << 2);
        }
        return nibble;
    }

    // 0x00500120
    constexpr std::array<uint32_t, 8> movementNibbleToDistance = {
        0,
        0x220C,
        0x220C,
        0x3027,
        0x199A,
        0x2A99,
        0x2A99,
        0x3689,
    };

    // 0x00500244
    constexpr std::array<World::TilePos2, 9> kNearbyTiles = {
        World::TilePos2{ 0, 0 },
        World::TilePos2{ 0, 1 },
        World::TilePos2{ 1, 1 },
        World::TilePos2{ 1, 0 },
        World::TilePos2{ 1, -1 },
        World::TilePos2{ 0, -1 },
        World::TilePos2{ -1, -1 },
        World::TilePos2{ -1, 0 },
        World::TilePos2{ -1, 1 },
    };

    // 0x004B1876
    static std::optional<EntityId> checkForCollisions(VehicleBogie& bogie, World::Pos3& loc)
    {
        if (bogie.mode != TransportMode::rail)
        {
            return std::nullopt;
        }

        Vehicle srcTrain(bogie.head);

        for (auto& nearby : kNearbyTiles)
        {
            const auto inspectionPos = World::toTileSpace(loc) + nearby;
            for (auto* entity : EntityManager::EntityTileList(World::toWorldSpace(inspectionPos)))
            {
                auto* vehicleBase = entity->asBase<VehicleBase>();
                if (vehicleBase == nullptr)
                {
                    continue;
                }
                if (vehicleBase->getTransportMode() != TransportMode::rail)
                {
                    continue;
                }

                const auto zDiff = std::abs(loc.z - vehicleBase->position.z);
                if (zDiff > 16)
                {
                    continue;
                }

                // vanilla did some overflow checks here but since we promote to int it shouldn't be needed
                const auto distance = Math::Vector::manhattanDistance2D(vehicleBase->position, loc);
                if (distance >= 12)
                {
                    continue;
                }

                const auto subType = vehicleBase->getSubType();
                // Does it actually have a collidable body
                if (subType != VehicleEntityType::body_continued && subType != VehicleEntityType::body_start && subType != VehicleEntityType::bogie)
                {
                    continue;
                }

                if (vehicleBase->owner != bogie.owner)
                {
                    continue;
                }

                // This is an optimisation compared to vanilla
                if (vehicleBase->getHead() != bogie.head)
                {
                    return vehicleBase->id;
                }

                bool noSelfCollision = false;
                {
                    Vehicle collideTrain(vehicleBase->getHead());
                    bool carFound = false;
                    uint32_t numCarsToCheck = 0;
                    for (auto& car : collideTrain.cars)
                    {
                        for (auto& carComponent : car)
                        {

                            if (!carFound)
                            {
                                carComponent.applyToComponents([&carFound, &numCarsToCheck, targetId = vehicleBase->id](auto& c) {
                                    if (c.id == targetId)
                                    {
                                        carFound = true;
                                        numCarsToCheck = 3;
                                    }
                                });
                            }
                            if (carFound)
                            {
                                carComponent.applyToComponents([&noSelfCollision, targetId = bogie.id](auto& c) {
                                    if (c.id == targetId)
                                    {
                                        noSelfCollision = true;
                                    }
                                });
                            }
                        }
                        if (noSelfCollision)
                        {
                            break;
                        }
                        if (carFound)
                        {
                            numCarsToCheck--;
                            if (numCarsToCheck == 0)
                            {
                                break;
                            }
                        }
                    }
                }
                if (noSelfCollision)
                {
                    continue;
                }
                {
                    bool carFound = false;
                    uint32_t numCarsToCheck = 0;
                    for (auto& car : srcTrain.cars)
                    {
                        for (auto& carComponent : car)
                        {

                            if (!carFound)
                            {
                                carComponent.applyToComponents([&carFound, &numCarsToCheck, targetId = bogie.id](auto& c) {
                                    if (c.id == targetId)
                                    {
                                        carFound = true;
                                        numCarsToCheck = 3;
                                    }
                                });
                            }
                            if (carFound)
                            {
                                carComponent.applyToComponents([&noSelfCollision, targetId = vehicleBase->id](auto& c) {
                                    if (c.id == targetId)
                                    {
                                        noSelfCollision = true;
                                    }
                                });
                            }
                        }
                        if (noSelfCollision)
                        {
                            break;
                        }
                        if (carFound)
                        {
                            numCarsToCheck--;
                            if (numCarsToCheck == 0)
                            {
                                break;
                            }
                        }
                    }
                }
                if (noSelfCollision)
                {
                    continue;
                }
                return vehicleBase->id;
            }
        }
        return std::nullopt;
    }

    // 0x0047C7FA
    static int32_t updateRoadMotion(VehicleCommon& component, int32_t distance)
    {
        component.remainingDistance += distance;
        bool hasMoved = false;
        auto returnValue = 0;
        auto intermediatePosition = component.position;
        while (component.remainingDistance >= 0x368A)
        {
            hasMoved = true;
            auto newSubPosition = component.subPosition + 1U;
            const auto subPositionDataSize = World::TrackData::getRoadSubPositon(component.trackAndDirection.road._data).size();
            // This means we have moved forward by a road piece
            if (newSubPosition >= subPositionDataSize)
            {
                if (!updateRoadMotionNewRoadPiece(component))
                {
                    returnValue = component.remainingDistance - 0x3689;
                    component.remainingDistance = 0x3689;
                    _vehicleUpdate_var_1136114 |= (1U << 0);
                    break;
                }
                else
                {
                    newSubPosition = 0;
                }
            }
            // 0x0047C95B
            component.subPosition = newSubPosition;
            const auto& moveData = World::TrackData::getRoadSubPositon(component.trackAndDirection.road._data)[newSubPosition];
            const auto nextNewPosition = moveData.loc + World::Pos3(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);
            component.remainingDistance -= movementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
            intermediatePosition = nextNewPosition;
            component.spriteYaw = moveData.yaw;
            component.spritePitch = moveData.pitch;
            if (component.getSubType() == VehicleEntityType::bogie)
            {
                // collision checks
                auto collideResult = checkForCollisions(component, intermediatePosition);
                if (collideResult.has_value())
                {
                    _vehicleUpdate_var_1136114 |= (1U << 2);
                    _vehicleUpdate_collisionCarComponent = collideResult.value();
                }
            }
        }
        if (hasMoved)
        {
            Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
            component.moveTo(intermediatePosition);
            Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
        }
        return returnValue;
    }

    static int32_t updateTrackMotion(VehicleCommon& component, int32_t distance)
    {
        if (component.mode == TransportMode::road)
        {
            return updateRoadMotion(component, distance);
        }
        else if (component.mode == TransportMode::rail)
        {
            component.remainingDistance += distance;
            bool hasMoved = false;
            auto returnValue = 0;
            auto intermediatePosition = component.position;
            while (component.remainingDistance >= 0x368A)
            {
                hasMoved = true;
                auto newSubPosition = component.subPosition + 1U;
                const auto subPositionDataSize = World::TrackData::getTrackSubPositon(component.trackAndDirection.track._data).size();
                // This means we have moved forward by a track piece
                if (newSubPosition >= subPositionDataSize)
                {
                    if (!updateTrackMotionNewTrackPiece(component))
                    {
                        returnValue = component.remainingDistance - 0x3689;
                        component.remainingDistance = 0x3689;
                        _vehicleUpdate_var_1136114 |= (1U << 0);
                        break;
                    }
                    else
                    {
                        newSubPosition = 0;
                    }
                }
                // 0x004B1761
                component.subPosition = newSubPosition;
                const auto& moveData = World::TrackData::getTrackSubPositon(component.trackAndDirection.track._data)[newSubPosition];
                const auto nextNewPosition = moveData.loc + World::Pos3(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);
                component.remainingDistance -= movementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
                intermediatePosition = nextNewPosition;
                component.spriteYaw = moveData.yaw;
                component.spritePitch = moveData.pitch;
                if (component.isVehicleBogie())
                {
                    // collision checks
                    auto collideResult = checkForCollisions(*component.asVehicleBogie(), intermediatePosition);
                    if (collideResult.has_value())
                    {
                        _vehicleUpdate_var_1136114 |= (1U << 2);
                        _vehicleUpdate_collisionCarComponent = collideResult.value();
                    }
                }
            }
            if (hasMoved)
            {
                Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
                component.moveTo(intermediatePosition);
                Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
            }
            return returnValue;
        }
        else
        {
            assert(false);
            return 0;
        }
    }

    // 0x004B15FF
    int32_t VehicleBase::updateTrackMotion(int32_t unk1)
    {
        return Vehicles::updateTrackMotion(*reinterpret_cast<VehicleCommon*>(this), unk1);
    }

    // 0x0047D959
    // ax : loc.x
    // cx : loc.y
    // dl : loc.z / 4
    // bp : trackAndDirection
    // ebp : bp | (setOccupied << 31)
    // returns dh : trackType
    uint8_t VehicleBase::sub_47D959(const World::Pos3& loc, const TrackAndDirection::_RoadAndDirection trackAndDirection, const bool setOccupied)
    {
        auto trackType = getTrackType();
        auto tile = World::TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }

            const auto heightDiff = std::abs(loc.z / 4 - elRoad->baseZ());
            if (heightDiff > 4)
            {
                continue;
            }

            if (elRoad->rotation() != trackAndDirection.cardinalDirection())
            {
                continue;
            }

            if (elRoad->roadId() != trackAndDirection.id())
            {
                continue;
            }

            if (elRoad->isAiAllocated())
            {
                continue;
            }

            const auto newUnk4u = _4F7358[trackAndDirection._data >> 2] >> 4;
            if (setOccupied)
            {
                elRoad->setUnk4u(elRoad->unk4u() | newUnk4u);
            }
            else
            {
                elRoad->setUnk4u(elRoad->unk4u() & (~newUnk4u));
            }

            if (getTrackType() == 0xFF)
            {
                if (getGameState().roadObjectIdIsNotTram & (1 << elRoad->roadObjectId()))
                {
                    elRoad->setUnk7_40(true);
                    trackType = elRoad->roadObjectId();
                }
            }
            else
            {
                trackType = getTrackType();
            }
        }
        return trackType;
    }

    bool VehicleBase::updateComponent()
    {
        switch (getSubType())
        {
            case VehicleEntityType::head:
                return !asVehicleHead()->update();
            case VehicleEntityType::vehicle_1:
                return !asVehicle1()->update();
            case VehicleEntityType::vehicle_2:
                return !asVehicle2()->update();
            case VehicleEntityType::bogie:
                return !asVehicleBogie()->update();
            case VehicleEntityType::body_start:
            case VehicleEntityType::body_continued:
                return !asVehicleBody()->update();
            case VehicleEntityType::tail:
                return !asVehicleTail()->update();
            default:
                break;
        }
        return false;
    }

    CarComponent::CarComponent(VehicleBase*& component)
    {
        front = component->asVehicleBogie();
        if (front == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        back = component->asVehicleBogie();
        if (back == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        body = component->asVehicleBody();
        if (body == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
    }

    Vehicle::Vehicle(EntityId _head)
    {
        auto component = EntityManager::get<VehicleBase>(_head);
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        head = component->asVehicleHead();
        if (head == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        veh1 = component->asVehicle1();
        if (veh1 == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        veh2 = component->asVehicle2();
        if (veh2 == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        if (component->getSubType() != VehicleEntityType::tail)
        {
            cars = Cars{ Car{ component } };
        }
        while (component->getSubType() != VehicleEntityType::tail)
        {
            component = component->nextVehicleComponent();
            if (component == nullptr)
            {
                throw Exception::RuntimeError("Bad vehicle structure");
            }
        }
        tail = component->asVehicleTail();
    }

    // 0x00426790
    AirportObjectFlags VehicleBogie::getCompatibleAirportType()
    {
        auto* vehObj = ObjectManager::get<VehicleObject>(objectId);
        if (vehObj->hasFlags(VehicleObjectFlags::isHelicopter))
        {
            return AirportObjectFlags::acceptsHelicopter;
        }
        if (vehObj->weight < 50)
        {
            return AirportObjectFlags::acceptsHeavyPlanes | AirportObjectFlags::acceptsLightPlanes;
        }
        return AirportObjectFlags::acceptsHeavyPlanes;
    }

    // 0x004AF16A
    void removeAllCargo(CarComponent& carComponent)
    {
        carComponent.front->secondaryCargo.qty = 0;
        carComponent.back->secondaryCargo.qty = 0;
        carComponent.body->primaryCargo.qty = 0;

        auto* head = EntityManager::get<VehicleHead>(carComponent.front->head);
        if (head == nullptr)
        {
            throw Exception::RuntimeError("Invalid Vehicle head");
        }
        head->sub_4B7CC3();

        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, enumValue(head->id));

        // Vanilla called updateCargoSprite on the bogies but that does nothing so skipping that.
        carComponent.body->updateCargoSprite();
    }

    void registerHooks()
    {
        registerHook(
            0x0047C7FA,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                uint32_t distance = regs.eax;
                VehicleCommon* component = X86Pointer<VehicleCommon>(regs.esi);

                const auto res = updateRoadMotion(*component, distance);

                regs = backup;
                regs.eax = res;
                return 0;
            });
    }
}
