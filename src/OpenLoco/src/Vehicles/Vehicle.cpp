#include "Vehicle.h"
#include "Effects/ExplosionEffect.h"
#include "Effects/VehicleCrashEffect.h"
#include "Entities/EntityManager.h"
#include "GameState.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/SubpositionData.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "MessageManager.h"
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
        ColourScheme colourScheme;           // 0x24
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

    ColourScheme VehicleBase::getColourScheme()
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        return veh->colourScheme;
    }

    void VehicleBase::setColourScheme(ColourScheme colourScheme)
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        veh->colourScheme = colourScheme;
    }

    VehicleBase* VehicleBase::nextVehicle()
    {
        return EntityManager::get<VehicleBase>(nextEntityId);
    }

    VehicleBase* VehicleBase::nextVehicleComponent()
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        return EntityManager::get<VehicleBase>(veh->nextCarId);
    }

    VehicleBase* VehicleBase::previousVehicleComponent()
    {
        auto head = EntityManager::get<VehicleBase>(this->getHead());
        while (head->nextVehicleComponent() != this)
        {
            head = head->nextVehicleComponent();
        }
        return head;
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

    EntityId VehicleBase::getNextCar() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->nextCarId;
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

    // 0x004AA407
    void VehicleBase::explodeComponent()
    {
        auto subType = getSubType();
        assert(subType == VehicleEntityType::bogie || subType == VehicleEntityType::body_start || subType == VehicleEntityType::body_continued);

        const auto pos = position + World::Pos3{ 0, 0, 22 };
        Audio::playSound(Audio::SoundId::crash, pos);

        ExplosionCloud::create(pos);

        const auto numParticles = std::min(spriteWidth / 4, 7);
        for (auto i = 0; i < numParticles; ++i)
        {
            ColourScheme colourScheme = (subType == VehicleEntityType::bogie) ? reinterpret_cast<VehicleBogie*>(this)->colourScheme : reinterpret_cast<VehicleBody*>(this)->colourScheme;
            VehicleCrashParticle::create(pos, colourScheme);
        }
    }

    // 0x004AA464
    void VehicleBase::destroyTrain()
    {
        Vehicle train(this->getHead());

        if (train.head->status != Status::crashed && train.head->status != Status::stuck)
        {
            train.head->status = Status::crashed;
            train.head->crashedTimeout = 0;

            if (train.head->owner == getGameState().playerCompanies[0])
            {
                MessageManager::post(
                    MessageType::vehicleCrashed,
                    train.head->owner,
                    (uint16_t)train.head->id,
                    0xFFFF,
                    0xFFFF);
            }
        }

        train.cars.applyToComponents([](auto& carComponent) { carComponent.refundCost = 0; });

        train.head->totalRefundCost = 0;

        Speed32 currentSpeed = train.veh2->currentSpeed;
        train.veh2->motorState = MotorState::stopped;

        train.cars.applyToComponents([&](auto& carComponent) {
            if (carComponent.isVehicleBogie())
            {
                carComponent.asVehicleBogie()->var_5A = currentSpeed.getRaw();
            }
        });

        if (this->getSubType() == VehicleEntityType::vehicle_2)
        {
            auto* bogie = train.cars.firstCar.front;

            if (!train.cars.empty())
            {
                bogie->var_5A |= (1U << 31);
                bogie->tileX = 0;
                bogie->tileY = 0;
                bogie->tileBaseZ = 0;
            }
        }
        else if (this->getSubType() == VehicleEntityType::bogie)
        {
            VehicleBogie* bogie = this->asVehicleBogie();

            bogie->var_5A |= (1U << 31);
            bogie->tileX = 0;
            bogie->tileY = 0;
            bogie->tileBaseZ = 0;
        }
        else
        {
            VehicleBogie* explodeBogie = nullptr;
            for (auto& car : train.cars)
            {
                for (auto& carComponent : car)
                {
                    explodeBogie = carComponent.back;
                    if (carComponent.body == this)
                    {
                        break;
                    }
                }
            }
            if (explodeBogie != nullptr)
            {
                explodeBogie->var_5A |= (1U << 31);
                explodeBogie->tileX = 0;
                explodeBogie->tileY = 0;
                explodeBogie->tileBaseZ = 0;
            }
        }
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
            if (_vehicleUpdate_var_1136114 & UpdateVar1136114Flags::unk_m15)
            {
                if (train.veh1->routingHandle == component.routingHandle)
                {
                    _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m03;
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
                _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::noRouteFound;
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
            if (_vehicleUpdate_var_1136114 & UpdateVar1136114Flags::unk_m15)
            {
                if (train.veh1->routingHandle == component.routingHandle)
                {
                    _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m03;
                    return false;
                }
            }
            World::Pos3 pos(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);

            auto [nextPos, nextRot] = World::Track::getTrackConnectionEnd(pos, component.trackAndDirection.track._data);
            const auto tc = World::Track::getTrackConnections(nextPos, nextRot, component.owner, component.trackType, train.head->var_53, 0);
            if (tc.hasLevelCrossing)
            {
                _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m04;
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
                _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::noRouteFound;
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

    static constexpr uint8_t getMovementNibble(const World::Pos3& pos1, const World::Pos3& pos2)
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
    static constexpr std::array<uint32_t, 8> movementNibbleToDistance = {
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
    static constexpr std::array<World::TilePos2, 9> kNearbyTiles = {
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

    // If candidate within 8 vehicle components of src we ignore a self collision
    // TODO: If we stored the car index this could be simplified
    static bool ignoreSelfCollision(VehicleBase& sourceVehicleId, const VehicleBase& candidateVehicleId)
    {
        auto* src = &sourceVehicleId;
        for (uint32_t i = 0; i < 8; ++i)
        {
            src = src->nextVehicleComponent();
            if (src == nullptr)
            {
                return false;
            }
            if (src == &candidateVehicleId)
            {
                return true;
            }
        }
        return false;
    }

    // 0x004B1876
    static EntityId checkForCollisions(VehicleBogie& bogie, World::Pos3& loc)
    {
        if (bogie.mode != TransportMode::rail)
        {
            return EntityId::null;
        }

        Vehicle srcTrain(bogie.head);

        for (const auto& nearby : kNearbyTiles)
        {
            const auto inspectionPos = World::toTileSpace(loc) + nearby;
            for (auto* entity : EntityManager::EntityTileList(World::toWorldSpace(inspectionPos)))
            {
                auto* vehicleBase = entity->asBase<VehicleBase>();
                if (vehicleBase == nullptr || vehicleBase == &bogie)
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

                if (ignoreSelfCollision(bogie, *vehicleBase))
                {
                    continue;
                }
                if (ignoreSelfCollision(*vehicleBase, bogie))
                {
                    continue;
                }
                return vehicleBase->id;
            }
        }
        return EntityId::null;
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
                    _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m00;
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
            if (component.isVehicleBogie())
            {
                // collision checks
                auto collideResult = checkForCollisions(*component.asVehicleBogie(), intermediatePosition);
                if (collideResult != EntityId::null)
                {
                    _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m02;
                    _vehicleUpdate_collisionCarComponent = collideResult;
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
                        _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m00;
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
                    if (collideResult != EntityId::null)
                    {
                        _vehicleUpdate_var_1136114 |= UpdateVar1136114Flags::unk_m02;
                        _vehicleUpdate_collisionCarComponent = collideResult;
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
        if (vehObj->hasFlags(VehicleObjectFlags::aircraftIsHelicopter))
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
        head->updateTrainProperties();

        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, enumValue(head->id));

        // Vanilla called updateCargoSprite on the bogies but that does nothing so skipping that.
        carComponent.body->updateCargoSprite();
    }

    // 0x004AFFF3
    // esi: frontBogie
    // returns new front bogie as esi
    VehicleBogie* flipCar(VehicleBogie& frontBogie)
    {
        Vehicle train(frontBogie.head);
        auto precedingVehicleComponent = frontBogie.previousVehicleComponent();
        CarComponent oldFirstComponent;
        sfl::static_vector<CarComponent, VehicleObject::kMaxCarComponents> components;

        for (auto& car : train.cars)
        {
            if (car.front->id != frontBogie.id)
            {
                continue;
            }
            oldFirstComponent = car;
            for (CarComponent& component : car)
            {
                std::swap(component.front->objectSpriteType, component.back->objectSpriteType);
                component.body->var_38 ^= Flags38::isReversed;
                components.push_back(component);
            }
            break;
        }

        // if the Car is only one CarComponent we don't have to swap any values
        if (components.size() == 1)
        {
            return &frontBogie;
        }
        CarComponent& newFirstComponent = components.back();
        newFirstComponent.body->setSubType(VehicleEntityType::body_start);
        precedingVehicleComponent->setNextCar(newFirstComponent.front->id);
        // set the new last component to point to the next car

        if (oldFirstComponent.body == nullptr)
        {
            throw Exception::RuntimeError("oldFirstComponent.body was nullptr");
        }
        oldFirstComponent.body->setNextCar(newFirstComponent.body->nextCarId);

        for (int i = components.size() - 2; i >= 0; i--)
        {
            components[i].body->setSubType(VehicleEntityType::body_continued);
            if (components[i + 1].body != nullptr)
            {
                components[i + 1].body->setNextCar(components[i].front->id);
            }
        }

        newFirstComponent.body->primaryCargo = oldFirstComponent.body->primaryCargo;
        newFirstComponent.body->breakdownFlags = oldFirstComponent.body->breakdownFlags;
        newFirstComponent.body->breakdownTimeout = oldFirstComponent.body->breakdownTimeout;
        newFirstComponent.front->secondaryCargo = oldFirstComponent.front->secondaryCargo;
        newFirstComponent.front->breakdownFlags = oldFirstComponent.front->breakdownFlags;
        newFirstComponent.front->breakdownTimeout = oldFirstComponent.front->breakdownTimeout;
        newFirstComponent.front->totalCarWeight = oldFirstComponent.front->totalCarWeight;
        newFirstComponent.front->reliability = oldFirstComponent.front->reliability;
        newFirstComponent.front->timeoutToBreakdown = oldFirstComponent.front->timeoutToBreakdown;

        // vanilla does not reset every value
        oldFirstComponent.body->primaryCargo.acceptedTypes = 0;
        oldFirstComponent.body->primaryCargo.type = 0xFF;
        oldFirstComponent.body->primaryCargo.maxQty = 0;
        oldFirstComponent.body->primaryCargo.qty = 0;
        oldFirstComponent.body->primaryCargo.numDays = 0;
        oldFirstComponent.front->secondaryCargo.acceptedTypes = 0;
        oldFirstComponent.front->secondaryCargo.type = 0xFF;
        oldFirstComponent.front->secondaryCargo.maxQty = 0;
        oldFirstComponent.front->secondaryCargo.qty = 0;
        oldFirstComponent.front->secondaryCargo.numDays = 0;

        oldFirstComponent.body->breakdownFlags = BreakdownFlags::none;
        oldFirstComponent.body->breakdownTimeout = 0;
        oldFirstComponent.front->breakdownFlags = BreakdownFlags::none;
        oldFirstComponent.front->breakdownTimeout = 0;

        return newFirstComponent.front;
    }

    // 0x004AF4D6
    // source: esi
    // dest: edi
    // returns nothing
    void insertCarBefore(VehicleBogie& source, VehicleBase& dest)
    {
        if (source.id == dest.id)
        {
            return;
        }
        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, enumValue(source.head));
        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, enumValue(dest.getHead()));
        Ui::WindowManager::invalidate(Ui::WindowType::vehicleList);

        Vehicle sourceTrain(source.head);
        auto precedingSourceComponent = source.previousVehicleComponent();
        for (auto& car : sourceTrain.cars)
        {
            if (car.front->id != source.id)
            {
                continue;
            }
            auto* lastBody = car.body;
            for (auto& component : car)
            {
                component.front->head = dest.getHead();
                component.back->head = dest.getHead();
                component.body->head = dest.getHead();
                lastBody = component.body;
            }
            precedingSourceComponent->setNextCar(lastBody->nextCarId);
            lastBody->nextCarId = dest.id;
            break;
        }
        auto precedingDestComponent = dest.previousVehicleComponent();
        precedingDestComponent->setNextCar(source.id);
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

        registerHook(
            0x004AFFF3,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                VehicleBogie* component = X86Pointer<VehicleBogie>(regs.esi);
                VehicleBogie* newComponent = flipCar(*component);
                regs = backup;
                regs.esi = X86Pointer(newComponent);
                return 0;
            });

        registerHook(
            0x004AF4D6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                VehicleBogie* source = X86Pointer<VehicleBogie>(regs.esi);
                VehicleBase* dest = X86Pointer<VehicleBase>(regs.edi);

                insertCarBefore(*source, *dest);

                regs = backup;
                return 0;
            });

        registerHook(
            0x00478CE9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                const auto pos = World::Pos3(regs.ax, regs.cx, regs.dx);
                const uint16_t tad = regs.bp;
                const auto companyId = CompanyId(regs.bl);
                const uint8_t roadObjId = regs.bh;
                const auto requiredMods = addr<0x0113601A, uint8_t>();
                const auto queryMods = addr<0x0113601B, uint8_t>();
                auto& legacyConnections = *X86Pointer<World::Track::LegacyTrackConnections>(regs.edi - 4);
                legacyConnections.size = 0;
                const auto [nextPos, nextRot] = World::Track::getRoadConnectionEnd(pos, tad);
                const auto connections = World::Track::getRoadConnectionsOneWay(nextPos, nextRot, companyId, roadObjId, requiredMods, queryMods);
                World::Track::toLegacyConnections(connections, legacyConnections);
                regs = backup;
                regs.ax = nextPos.x;
                regs.cx = nextPos.y;
                regs.dx = nextPos.z;
                return 0;
            });

        registerHook(
            0x00478AC9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                const auto pos = World::Pos3(regs.ax, regs.cx, regs.dx);
                const uint16_t tad = regs.bp;
                const auto companyId = CompanyId(regs.bl);
                const uint8_t roadObjId = regs.bh;
                const auto requiredMods = addr<0x0113601A, uint8_t>();
                const auto queryMods = addr<0x0113601B, uint8_t>();
                auto& legacyConnections = *X86Pointer<World::Track::LegacyTrackConnections>(regs.edi - 4);
                legacyConnections.size = 0;
                const auto [nextPos, nextRot] = World::Track::getRoadConnectionEnd(pos, tad);
                const auto connections = World::Track::getRoadConnectionsAiAllocated(nextPos, nextRot, companyId, roadObjId, requiredMods, queryMods);
                World::Track::toLegacyConnections(connections, legacyConnections);
                regs = backup;
                regs.ax = nextPos.x;
                regs.cx = nextPos.y;
                regs.dx = nextPos.z;
                return 0;
            });
    }
}
