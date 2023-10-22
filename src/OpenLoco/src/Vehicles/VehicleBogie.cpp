#include "Entities/EntityManager.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<VehicleBogie*, 0x01136124> _vehicleUpdate_frontBogie;
    static loco_global<VehicleBogie*, 0x01136128> _vehicleUpdate_backBogie;
    static loco_global<bool, 0x01136237> _vehicleUpdate_frontBogieHasMoved; // remainingDistance related?
    static loco_global<bool, 0x01136238> _vehicleUpdate_backBogieHasMoved;  // remainingDistance related?
    static loco_global<int32_t, 0x0113612C> _vehicleUpdate_var_113612C;     // Speed
    static loco_global<uint32_t, 0x01136114> _vehicleUpdate_var_1136114;
    static loco_global<int32_t, 0x01136130> _vehicleUpdate_var_1136130; // Speed
    static loco_global<EntityId, 0x0113610E> _vehicleUpdate_collisionCarComponent;

    template<typename T>
    void applyDestructionToComponent(T& component)
    {
        component.explodeComponent();
        component.var_5A &= ~(1u << 31);
        component.var_5A >>= 3;
        component.var_5A |= (1u << 31);
    }

    // 0x004AA008
    bool VehicleBogie::update()
    {
        _vehicleUpdate_frontBogie = _vehicleUpdate_backBogie;
        _vehicleUpdate_backBogie = this;

        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }

        const auto oldPos = position;
        _vehicleUpdate_var_1136114 = 0;
        updateTrackMotion(_vehicleUpdate_var_113612C);

        const auto hasMoved = oldPos != position;
        _vehicleUpdate_backBogieHasMoved = _vehicleUpdate_frontBogieHasMoved;
        _vehicleUpdate_frontBogieHasMoved = hasMoved;

        const int32_t stash1136130 = _vehicleUpdate_var_1136130;
        if (var_5E != 0)
        {
            auto unk = var_5E;
            if (unk > 32)
            {
                unk = 64 - unk;
            }
            _vehicleUpdate_var_1136130 = 500 + unk * 320;
        }

        updateRoll();
        _vehicleUpdate_var_1136130 = stash1136130;
        if (_vehicleUpdate_var_1136114 & (1 << 1))
        {
            sub_4AA464();
            return false;
        }
        else if (!(_vehicleUpdate_var_1136114 & (1 << 2)))
        {
            return true;
        }

        collision();
        return false;
    }

    // 0x004AAC02
    void VehicleBogie::updateRoll()
    {
        auto unk = _vehicleUpdate_var_1136130 / 8;
        if (has38Flags(Flags38::isReversed))
        {
            unk = -unk;
        }
        var_44 += unk;

        if (objectSpriteType != 0xFF)
        {
            auto* vehObj = ObjectManager::get<VehicleObject>(objectId);
            auto& bogieSprites = vehObj->bogieSprites[objectSpriteType];
            const auto newRoll = (bogieSprites.rollStates - 1) & (var_44 / 4096);
            if (newRoll != var_46)
            {
                var_46 = newRoll;
                invalidateSprite();
            }
        }
    }

    // 0x004AA0DF
    void VehicleBogie::collision()
    {
        sub_4AA464();
        applyDestructionToComponent(*this);
        vehicleFlags |= VehicleFlags::unk_5;

        // Apply collision to the whole car
        Vehicle train(head);
        bool end = false;
        for (auto& car : train.cars)
        {
            for (auto& carComponent : car)
            {
                if (carComponent.front == this || carComponent.back == this)
                {
                    applyDestructionToComponent(*carComponent.body);
                    end = true;
                    break;
                }
            }
            if (end)
            {
                break;
            }
        }

        // Apply Collision to collided train
        auto* collideEntity = EntityManager::get<EntityBase>(_vehicleUpdate_collisionCarComponent);
        auto* collideCarComponent = collideEntity->asBase<VehicleBase>();
        if (collideCarComponent != nullptr)
        {
            Vehicle collideTrain(collideCarComponent->getHead());
            if (collideTrain.head->status != Status::crashed)
            {
                collideCarComponent->sub_4AA464();
            }

            for (auto& car : train.cars)
            {
                for (auto& carComponent : car)
                {
                    if (carComponent.front == collideCarComponent)
                    {
                        applyDestructionToComponent(*carComponent.front);
                    }
                    if (carComponent.back == collideCarComponent)
                    {
                        applyDestructionToComponent(*carComponent.back);
                    }
                    if (carComponent.front == collideCarComponent || carComponent.back == collideCarComponent || carComponent.body == collideCarComponent)
                    {
                        applyDestructionToComponent(*carComponent.body);
                        return;
                    }
                }
            }
        }
    }

    // 0x004AA97A
    bool VehicleBogie::isOnRackRail()
    {
        if (mode == TransportMode::rail)
        {
            auto* trackObj = ObjectManager::get<TrackObject>(trackType);
            if (!trackObj->hasFlags(TrackObjectFlags::unk_00))
            {
                return true;
            }
            auto* vehObj = ObjectManager::get<VehicleObject>(objectId);
            if (!vehObj->hasFlags(VehicleObjectFlags::rackRail))
            {
                return false;
            }

            const auto tile = World::TileManager::get(World::Pos2{ tileX, tileY });
            for (auto& el : tile)
            {
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }

                if (elTrack->baseZ() != tileBaseZ)
                {
                    continue;
                }

                if (elTrack->unkDirection() != trackAndDirection.track.cardinalDirection())
                {
                    continue;
                }

                if (elTrack->trackId() != trackAndDirection.track.id())
                {
                    continue;
                }

                for (auto i = 0; i < 4; ++i)
                {
                    if (elTrack->hasMod(i) && trackObj->mods[i] == vehObj->rackRailType)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        else if (mode == TransportMode::road)
        {
            if (trackType == 0xFFU)
            {
                return true;
            }
            auto* roadObj = ObjectManager::get<RoadObject>(trackType);
            if (!roadObj->hasFlags(RoadObjectFlags::unk_05))
            {
                return true;
            }

            auto* vehObj = ObjectManager::get<VehicleObject>(objectId);
            if (!vehObj->hasFlags(VehicleObjectFlags::rackRail))
            {
                return false;
            }

            const auto tile = World::TileManager::get(World::Pos2{ tileX, tileY });
            for (auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }

                if (elRoad->baseZ() != tileBaseZ)
                {
                    continue;
                }

                if (elRoad->unkDirection() != trackAndDirection.road.cardinalDirection())
                {
                    continue;
                }

                if (elRoad->roadId() != trackAndDirection.road.id())
                {
                    continue;
                }

                for (auto i = 0; i < 2; ++i)
                {
                    if (elRoad->hasMod(i) && roadObj->mods[i] == vehObj->rackRailType)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        else
        {
            assert(false);
            return true;
        }
    }

    // 0x004AF16A
    void VehicleBogie::carComponent_sub_4AF16A()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        call(0x004AF16A, regs);
    }
}
