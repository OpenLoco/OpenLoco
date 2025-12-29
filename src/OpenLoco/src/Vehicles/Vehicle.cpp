#include "Vehicle.h"
#include "Audio/Audio.h"
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
#include "Objects/RoadObject.h"
#include "RoutingManager.h"
#include "Ui/WindowManager.h"
#include "Vehicle1.h"
#include "Vehicle2.h"
#include "VehicleBody.h"
#include "VehicleBogie.h"
#include "VehicleHead.h"
#include "VehicleTail.h"
#include "ViewportManager.h"
#include <OpenLoco/Core/Exception.hpp>

namespace OpenLoco::Vehicles
{
    static constexpr int32_t kObjDistToHighPrecisionDistance = 2179;
    // TODO: Get rid of this global
    static VehicleUpdateDistances _vehicleUpdateDistances = {};

    VehicleBase* VehicleBase::nextVehicle()
    {
        return EntityManager::get<VehicleBase>(nextEntityId);
    }

    VehicleBase* VehicleBase::nextVehicleComponent()
    {
        return EntityManager::get<VehicleBase>(nextCarId);
    }

    VehicleBase* VehicleBase::previousVehicleComponent()
    {
        auto component = EntityManager::get<VehicleBase>(this->getHead());
        while (component->nextVehicleComponent() != this)
        {
            component = component->nextVehicleComponent();
        }
        return component;
    }

    VehicleSound* VehicleBase::getVehicleSound()
    {
        if (is<VehicleEntityType::vehicle_2>())
        {
            return &as<Vehicle2>()->sound;
        }
        else if (is<VehicleEntityType::tail>())
        {
            return &as<VehicleTail>()->sound;
        }
        return nullptr;
    }

    TransportMode VehicleBase::getTransportMode() const
    {
        return mode;
    }

    Flags38 VehicleBase::getFlags38() const
    {
        return var_38;
    }

    uint8_t VehicleBase::getTrackType() const
    {
        return trackType;
    }

    World::Pos3 VehicleBase::getTrackLoc() const
    {
        return World::Pos3(tileX, tileY, tileBaseZ * World::kSmallZStep);
    }

    TrackAndDirection VehicleBase::getTrackAndDirection() const
    {
        return trackAndDirection;
    }

    RoutingHandle VehicleBase::getRoutingHandle() const
    {
        return routingHandle;
    }

    EntityId VehicleBase::getHead() const
    {
        return head;
    }

    int32_t VehicleBase::getRemainingDistance() const
    {
        return remainingDistance;
    }

    void VehicleBase::setNextCar(const EntityId newNextCar)
    {
        nextCarId = newNextCar;
    }

    EntityId VehicleBase::getNextCar() const
    {
        return nextCarId;
    }

    bool VehicleBase::has38Flags(Flags38 flagsToTest) const
    {
        return (var_38 & flagsToTest) != Flags38::none;
    }

    bool VehicleBase::hasVehicleFlags(VehicleFlags flagsToTest) const
    {
        return (vehicleFlags & flagsToTest) != VehicleFlags::none;
    }

    bool VehicleBase::isVehicleHead() const { return is<VehicleEntityType::head>(); }
    VehicleHead* VehicleBase::asVehicleHead() const { return as<VehicleHead>(); }
    bool VehicleBase::isVehicle1() const { return is<VehicleEntityType::vehicle_1>(); }
    Vehicle1* VehicleBase::asVehicle1() const { return as<Vehicle1>(); }
    bool VehicleBase::isVehicle2() const { return is<VehicleEntityType::vehicle_2>(); }
    Vehicle2* VehicleBase::asVehicle2() const { return as<Vehicle2>(); }
    bool VehicleBase::isVehicleBogie() const { return is<VehicleEntityType::bogie>(); }
    VehicleBogie* VehicleBase::asVehicleBogie() const { return as<VehicleBogie>(); }
    bool VehicleBase::isVehicleBody() const { return is<VehicleEntityType::body_start>() || is<VehicleEntityType::body_continued>(); }
    VehicleBody* VehicleBase::asVehicleBody() const
    {
        if (is<VehicleEntityType::body_start>())
        {
            return as<VehicleBody, VehicleEntityType::body_start>();
        }

        return as<VehicleBody, VehicleEntityType::body_continued>();
    }
    bool VehicleBase::hasSoundPlayer() { return is<VehicleEntityType::vehicle_2>() || is<VehicleEntityType::tail>(); }
    bool VehicleBase::isVehicleTail() const { return is<VehicleEntityType::tail>(); }
    VehicleTail* VehicleBase::asVehicleTail() const { return as<VehicleTail>(); }

    VehicleUpdateDistances& getVehicleUpdateDistances()
    {
        return _vehicleUpdateDistances;
    }

    // 0x004AA407
    void VehicleBase::explodeComponent()
    {
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

    static bool updateRoadMotionNewRoadPiece(VehicleBase& component, UpdateVar1136114Flags& flags, bool isVeh2UnkM15)
    {
        auto newRoutingHandle = component.routingHandle;
        auto newIndex = newRoutingHandle.getIndex() + 1;
        newRoutingHandle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(newRoutingHandle);
        if (routing != RoutingManager::kAllocatedButFreeRouting)
        {
            Vehicle train(component.head);
            if (isVeh2UnkM15)
            {
                if (train.veh1->routingHandle == component.routingHandle)
                {
                    flags |= UpdateVar1136114Flags::unk_m03;
                    return false;
                }
            }
            World::Pos3 pos(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);

            auto [nextPos, nextRot] = World::Track::getRoadConnectionEnd(pos, component.trackAndDirection.road.basicRad());
            const auto tc = World::Track::getRoadConnections(nextPos, nextRot, component.owner, component.trackType, train.head->var_53, 0);

            bool routingFound = false;
            for (auto& connection : tc.connections)
            {
                if ((connection & World::Track::AdditionalTaDFlags::basicRaDMask) == (routing & World::Track::AdditionalTaDFlags::basicRaDMask))
                {
                    routingFound = true;
                    break;
                }
            }
            if (!routingFound)
            {
                flags |= UpdateVar1136114Flags::noRouteFound;
                return false;
            }
            component.routingHandle = newRoutingHandle;
            const auto oldTaD = component.trackAndDirection.road._data;
            component.trackAndDirection.road._data = routing & 0x1FF;
            if (component.isVehicle2())
            {
                component.asVehicle2()->var_4F = tc.roadObjectId;
            }

            pos += World::TrackData::getUnkRoad(oldTaD & World::Track::AdditionalTaDFlags::basicRaDMask).pos;
            component.tileX = pos.x;
            component.tileY = pos.y;
            component.tileBaseZ = pos.z / World::kSmallZStep;
            return true;
        }

        return false;
    }

    static bool updateTrackMotionNewTrackPiece(VehicleBase& component, UpdateVar1136114Flags& flags, bool isVeh2UnkM15)
    {
        auto newRoutingHandle = component.routingHandle;
        auto newIndex = newRoutingHandle.getIndex() + 1;
        newRoutingHandle.setIndex(newIndex);
        const auto routing = RoutingManager::getRouting(newRoutingHandle);
        if (routing != RoutingManager::kAllocatedButFreeRouting)
        {
            Vehicle train(component.head);
            if (isVeh2UnkM15)
            {
                if (train.veh1->routingHandle == component.routingHandle)
                {
                    flags |= UpdateVar1136114Flags::unk_m03;
                    return false;
                }
            }
            World::Pos3 pos(component.tileX, component.tileY, component.tileBaseZ * World::kSmallZStep);

            auto [nextPos, nextRot] = World::Track::getTrackConnectionEnd(pos, component.trackAndDirection.track._data);
            const auto tc = World::Track::getTrackConnections(nextPos, nextRot, component.owner, component.trackType, train.head->var_53, 0);
            if (tc.hasLevelCrossing)
            {
                flags |= UpdateVar1136114Flags::approachingGradeCrossing;
            }
            bool routingFound = false;
            for (auto& connection : tc.connections)
            {
                if ((connection & World::Track::AdditionalTaDFlags::basicTaDMask) == (routing & World::Track::AdditionalTaDFlags::basicTaDMask))
                {
                    routingFound = true;
                    break;
                }
            }
            if (!routingFound)
            {
                flags |= UpdateVar1136114Flags::noRouteFound;
                return false;
            }
            component.routingHandle = newRoutingHandle;
            const auto oldTaD = component.trackAndDirection.track._data;
            component.trackAndDirection.track._data = routing & World::Track::AdditionalTaDFlags::basicTaDMask;
            pos += World::TrackData::getUnkTrack(oldTaD).pos;
            component.tileX = pos.x;
            component.tileY = pos.y;
            component.tileBaseZ = pos.z / World::kSmallZStep;
            return true;
        }

        return false;
    }

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
    EntityId checkForCollisions(VehicleBogie& bogie, World::Pos3& loc)
    {
        if (bogie.mode != TransportMode::rail)
        {
            return EntityId::null;
        }

        Vehicle srcTrain(bogie.head);

        for (const auto& nearby : kMooreNeighbourhood)
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
    static UpdateMotionResult updateRoadMotion(VehicleBase& component, int32_t distance, bool isVeh2UnkM15)
    {
        UpdateMotionResult result{};
        component.remainingDistance += distance;
        bool hasMoved = false;
        auto intermediatePosition = component.position;
        while (component.remainingDistance >= 0x368A)
        {
            hasMoved = true;
            auto newSubPosition = component.subPosition + 1U;
            const auto subPositionDataSize = World::TrackData::getRoadSubPositon(component.trackAndDirection.road._data).size();
            // This means we have moved forward by a road piece
            if (newSubPosition >= subPositionDataSize)
            {
                if (!updateRoadMotionNewRoadPiece(component, result.flags, isVeh2UnkM15))
                {
                    result.remainingDistance = component.remainingDistance - 0x3689;
                    component.remainingDistance = 0x3689;
                    result.flags |= UpdateVar1136114Flags::unk_m00;
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
            component.remainingDistance -= kMovementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
            intermediatePosition = nextNewPosition;
            component.spriteYaw = moveData.yaw;
            component.spritePitch = moveData.pitch;
            if (component.isVehicleBogie())
            {
                // collision checks
                auto collideResult = checkForCollisions(*component.asVehicleBogie(), intermediatePosition);
                if (collideResult != EntityId::null)
                {
                    result.flags |= UpdateVar1136114Flags::crashed;
                    result.collidedEntityId = collideResult;
                }
            }
        }
        if (hasMoved)
        {
            Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
            component.moveTo(intermediatePosition);
            Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
        }
        return result;
    }

    static UpdateMotionResult updateTrackMotion(VehicleBase& component, int32_t distance, bool isVeh2UnkM15)
    {
        if (component.mode == TransportMode::road)
        {
            return updateRoadMotion(component, distance, isVeh2UnkM15);
        }
        else if (component.mode == TransportMode::rail)
        {
            UpdateMotionResult result{};
            component.remainingDistance += distance;
            bool hasMoved = false;
            auto intermediatePosition = component.position;
            while (component.remainingDistance >= 0x368A)
            {
                hasMoved = true;
                auto newSubPosition = component.subPosition + 1U;
                const auto subPositionDataSize = World::TrackData::getTrackSubPositon(component.trackAndDirection.track._data).size();
                // This means we have moved forward by a track piece
                if (newSubPosition >= subPositionDataSize)
                {
                    if (!updateTrackMotionNewTrackPiece(component, result.flags, isVeh2UnkM15))
                    {
                        result.remainingDistance = component.remainingDistance - 0x3689;
                        component.remainingDistance = 0x3689;
                        result.flags |= UpdateVar1136114Flags::unk_m00;
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
                component.remainingDistance -= kMovementNibbleToDistance[getMovementNibble(intermediatePosition, nextNewPosition)];
                intermediatePosition = nextNewPosition;
                component.spriteYaw = moveData.yaw;
                component.spritePitch = moveData.pitch;
                if (component.isVehicleBogie())
                {
                    // collision checks
                    auto collideResult = checkForCollisions(*component.asVehicleBogie(), intermediatePosition);
                    if (collideResult != EntityId::null)
                    {
                        result.flags |= UpdateVar1136114Flags::crashed;
                        result.collidedEntityId = collideResult;
                    }
                }
            }
            if (hasMoved)
            {
                Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
                component.moveTo(intermediatePosition);
                Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
            }
            return result;
        }
        else
        {
            assert(false);
            return {};
        }
    }

    // 0x004B15FF
    UpdateMotionResult VehicleBase::updateTrackMotion(int32_t unk1, bool isVeh2UnkM15)
    {
        return Vehicles::updateTrackMotion(*this, unk1, isVeh2UnkM15);
    }

    // 0x0047D959
    // ax : loc.x
    // cx : loc.y
    // dl : loc.z / 4
    // bp : trackAndDirection
    // ebp : bp | (setOccupied << 31)
    // returns dh : trackType
    uint8_t VehicleBase::updateRoadTileOccupancy(const World::Pos3& loc, const TrackAndDirection::_RoadAndDirection rad, const bool setOccupied)
    {
        auto roadType = getTrackType();
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

            if (elRoad->rotation() != rad.cardinalDirection())
            {
                continue;
            }

            if (elRoad->roadId() != rad.id())
            {
                continue;
            }

            if (elRoad->isAiAllocated())
            {
                continue;
            }

            const auto newLaneOccupation = World::TrackData::getRoadOccupationMask(rad._data >> 2) >> 4;
            if (setOccupied)
            {
                elRoad->setLaneOccupation(elRoad->laneOccupation() | newLaneOccupation);
            }
            else
            {
                elRoad->setLaneOccupation(elRoad->laneOccupation() & (~newLaneOccupation));
            }

            if (getTrackType() == 0xFF)
            {
                if (getGameState().roadObjectIdIsNotTram & (1 << elRoad->roadObjectId()))
                {
                    elRoad->setUnk7_40(true);
                    roadType = elRoad->roadObjectId();
                }
            }
            else
            {
                roadType = getTrackType();
            }
        }
        return roadType;
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

    void Vehicle::refreshCars()
    {
        auto component = veh2->nextVehicleComponent();
        if (component == nullptr)
        {
            throw Exception::RuntimeError("Bad vehicle structure");
        }
        if (component->getSubType() != VehicleEntityType::tail)
        {
            cars = Cars{ Car{ component } };
        }
        else
        {
            cars = Cars{};
        }
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

        for (auto i = static_cast<int32_t>(components.size()) - 2; i >= 0; i--)
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

    // 0x004AF5E1
    // esi: head
    // returns nothing
    void connectJacobsBogies(VehicleHead& head)
    {
        /*
        Jacobs Bogie connections are completely invisible until a jacob's bogie connection is made. The visible body of the car is another
        CarComponent that is either the first CarComponent (if a connection is available only at the back of the Car), the middle CarComponent
        (if there are two connections available), or the last CarComponent (if a connection is available only at the front of the Car).
        The comments in the code assume that two connections are available, and refer to each CarComponent as the First, Last, and Body CarComponent.

        The bodies of these invisible CarComponents are used to store the flag that the jacob's bogie connection is available. If the first
        CarComponent of a Car sees that the previous CarComponent has the flag, and it has the flag as well, a connection will be made.

        If the last CarComponent of a Car is flagged for a connection, it will reset to default without checking if the next component should
        connect, and the connection will be re-made by the next Car, if necessary. This is eaiser than verifying that the connection is made
        correctly.
        */
        Vehicle train(head);
        auto componentsFound = 0;
        CarComponent previousCarComponent;
        CarComponent secondPreviousCarComponent;
        for (auto& car : train.cars)
        {
            if (car.body->has38Flags(Flags38::jacobsBogieAvailable))
            {
                auto frontBogieOfNext = car.body->nextVehicleComponent();
                if (frontBogieOfNext == nullptr)
                {
                    throw Exception::RuntimeError("connectJacobsBogies frontBogieOfNext was unexpectedly nullptr");
                }
                // Body's component
                CarComponent nextComponent = CarComponent(frontBogieOfNext);

                // Create First's jacob's bogie connection
                // Change from vanilla: this case occurred after the code in the else-block and the else-block was not conditional.
                if (componentsFound >= 1 && previousCarComponent.body->has38Flags(Flags38::jacobsBogieAvailable))
                {
                    if (componentsFound < 2)
                    {
                        throw Exception::RuntimeError("connectJacobsBogies tried to connect jacob's bogie without secondPreviousCarComponent");
                    }

                    auto frontObject = ObjectManager::get<VehicleObject>(car.front->objectId);
                    car.front->objectSpriteType = frontObject->carComponents[car.front->bodyIndex].frontBogieSpriteInd;
                    // set my body's front bogie to invisible
                    nextComponent.front->objectSpriteType = 0xFF;
                    // set previous car's body's rear bogie to invisible
                    secondPreviousCarComponent.back->objectSpriteType = 0xFF;
                }
                // Reset First's jacob's bogie connection
                else
                {
                    car.front->objectSpriteType = 0xFF;
                    car.back->objectSpriteType = 0xFF;
                    car.body->objectSpriteType = 0xFF;

                    auto bodyObject = ObjectManager::get<VehicleObject>(nextComponent.body->objectId);
                    nextComponent.front->objectSpriteType = bodyObject->carComponents[nextComponent.body->bodyIndex].frontBogieSpriteInd;
                    if (nextComponent.body->has38Flags(Flags38::isReversed))
                    {
                        // Change from vanilla: set bogie orientation based on body's object
                        nextComponent.front->objectSpriteType = bodyObject->carComponents[nextComponent.body->bodyIndex].backBogieSpriteInd;
                    }
                }
            }
            for (auto& component : car)
            {
                // Reset Last jacob's bogie connection
                // Jacobs bogie flag is only set on the first and last CarComponent of the car, it cannot be set on middle one(s)
                if (component.body->has38Flags(Flags38::jacobsBogieAvailable) && component.body->getSubType() == VehicleEntityType::body_continued)
                {
                    if (componentsFound == 0)
                    {
                        throw Exception::RuntimeError("connectJacobsBogies reached end of Car without previousCarComponent");
                    }
                    component.front->objectSpriteType = 0xFF;
                    component.back->objectSpriteType = 0xFF;
                    component.body->objectSpriteType = 0xFF;
                    // Change from vanilla: gets bogie's object instead of body's object
                    auto carLastBogieObject = ObjectManager::get<VehicleObject>(previousCarComponent.back->objectId);
                    previousCarComponent.back->objectSpriteType = carLastBogieObject->carComponents[previousCarComponent.back->bodyIndex].backBogieSpriteInd;
                    if (previousCarComponent.body->has38Flags(Flags38::isReversed))
                    {
                        // Change from vanilla: sets bogie orientation based on body's object
                        previousCarComponent.back->objectSpriteType = carLastBogieObject->carComponents[previousCarComponent.back->bodyIndex].frontBogieSpriteInd;
                    }
                }
                secondPreviousCarComponent = previousCarComponent;
                previousCarComponent = component;
                componentsFound++;
            }
        }
    }

    // 0x004B1C48
    // Applies the vehicle object lengths to the bogies of the train
    // with some specified starting distance. Bodies are not set as
    // they are calculated later based on the positions of the bogies.
    // returns the final distance
    static int32_t applyVehicleObjectLengthToBogies(Vehicle& train, const int32_t startDistance)
    {
        auto distance = startDistance;
        for (auto& car : train.cars)
        {
            const auto* vehicleObj = ObjectManager::get<VehicleObject>(car.front->objectId);
            assert(std::distance(car.begin(), car.end()) == vehicleObj->numCarComponents);
            if (car.body->has38Flags(Flags38::isReversed))
            {
                auto objCarIndex = vehicleObj->numCarComponents - 1;
                for (auto& component : car)
                {
                    auto& objCar = vehicleObj->carComponents[objCarIndex];
                    const auto frontLength = objCar.backBogiePosition * -kObjDistToHighPrecisionDistance;
                    component.front->remainingDistance = distance + frontLength;

                    if (objCar.bodySpriteInd != 0xFFU)
                    {
                        const auto bodyLength = vehicleObj->bodySprites[objCar.bodySpriteInd & 0x7F].halfLength * -(kObjDistToHighPrecisionDistance * 2);
                        distance += bodyLength;
                    }
                    const auto backLength = objCar.frontBogiePosition * kObjDistToHighPrecisionDistance;
                    component.back->remainingDistance = distance + backLength;

                    objCarIndex--;
                }
            }
            else
            {
                auto objCarIndex = 0;
                for (auto& component : car)
                {
                    auto& objCar = vehicleObj->carComponents[objCarIndex];
                    const auto frontLength = objCar.frontBogiePosition * -kObjDistToHighPrecisionDistance;
                    component.front->remainingDistance = distance + frontLength;

                    if (objCar.bodySpriteInd != 0xFFU)
                    {
                        const auto bodyLength = vehicleObj->bodySprites[objCar.bodySpriteInd & 0x7F].halfLength * -(kObjDistToHighPrecisionDistance * 2);
                        distance += bodyLength;
                    }
                    const auto backLength = objCar.backBogiePosition * kObjDistToHighPrecisionDistance;
                    component.back->remainingDistance = distance + backLength;

                    objCarIndex++;
                }
            }
        }
        return distance;
    }

    // 0x004AE2AB
    // head: esi
    void applyVehicleObjectLength(Vehicle& train)
    {
        // We want the tail to have a remaining distance of 0
        // so we first apply the lengths from 0 then take the return
        // length and set that as the negative start offset and reapply

        const auto negStartDistance = -applyVehicleObjectLengthToBogies(train, 0);
        train.head->remainingDistance = negStartDistance;
        train.veh1->remainingDistance = negStartDistance;
        train.veh2->remainingDistance = negStartDistance;
        applyVehicleObjectLengthToBogies(train, negStartDistance);
        train.tail->remainingDistance = 0;
    }

    Car::CarComponentIter::CarComponentIter(const CarComponent* carComponent)
    {
        if (carComponent == nullptr)
        {
            nextVehicleComponent = nullptr;
            return;
        }
        current = *carComponent;
        nextVehicleComponent = current.body->nextVehicleComponent();
    }

    Car::CarComponentIter& Car::CarComponentIter::operator++()
    {
        if (nextVehicleComponent == nullptr)
        {
            return *this;
        }
        if (nextVehicleComponent->getSubType() == VehicleEntityType::tail)
        {
            nextVehicleComponent = nullptr;
            return *this;
        }
        CarComponent next{ nextVehicleComponent };
        if (next.body == nullptr || next.body->getSubType() == VehicleEntityType::body_start)
        {
            nextVehicleComponent = nullptr;
            return *this;
        }
        current = next;
        return *this;
    }

    Vehicle::Cars::CarIter::CarIter(const Car* carComponent)
    {
        if (carComponent == nullptr || carComponent->body == nullptr)
        {
            nextVehicleComponent = nullptr;
            return;
        }
        current = *carComponent;
        nextVehicleComponent = current.body->nextVehicleComponent();
    }

    Vehicle::Cars::CarIter& Vehicle::Cars::CarIter::operator++()
    {
        if (nextVehicleComponent == nullptr)
        {
            return *this;
        }
        while (nextVehicleComponent->getSubType() != VehicleEntityType::tail)
        {
            Car next{ nextVehicleComponent };
            if (next.body == nullptr)
            {
                break;
            }
            if (next.body->getSubType() == VehicleEntityType::body_start)
            {
                current = next;
                return *this;
            }
        }
        nextVehicleComponent = nullptr;
        return *this;
    }

    Vehicle::Vehicle(const VehicleHead& _head)
        : Vehicle(_head.id)
    {
    }
}
