#include "Effects/ExplosionEffect.h"
#include "Effects/VehicleCrashEffect.h"
#include "Entities/EntityManager.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
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

    // 0x004AA407
    template<typename T>
    void explodeComponent(T& component)
    {
        assert(component.getSubType() == VehicleEntityType::bogie || component.getSubType() == VehicleEntityType::body_start || component.getSubType() == VehicleEntityType::body_continued);

        const auto pos = component.position + World::Pos3{ 0, 0, 22 };
        Audio::playSound(Audio::SoundId::crash, pos);

        ExplosionCloud::create(pos);

        const auto numParticles = std::min(component.spriteWidth / 4, 7);
        for (auto i = 0; i < numParticles; ++i)
        {
            VehicleCrashParticle::create(pos, component.colourScheme);
        }
    }

    template<typename T>
    void applyDestructionToComponent(T& component)
    {
        explodeComponent(component);
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
        if (wheelSlipping != 0)
        {
            auto unk = wheelSlipping;
            if (unk > kWheelSlippingDuration / 2)
            {
                unk = kWheelSlippingDuration - unk;
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
            const auto newAnimationIndex = (bogieSprites.numAnimationFrames - 1) & (var_44 / 4096);
            if (newAnimationIndex != animationIndex)
            {
                animationIndex = newAnimationIndex;
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

    // 0x004AA984
    static bool isOnRackRailRail(const VehicleBogie& bogie)
    {
        auto* trackObj = ObjectManager::get<TrackObject>(bogie.trackType);
        if (!trackObj->hasFlags(TrackObjectFlags::unk_00))
        {
            return true;
        }
        auto* vehObj = ObjectManager::get<VehicleObject>(bogie.objectId);
        if (!vehObj->hasFlags(VehicleObjectFlags::rackRail))
        {
            return false;
        }

        const auto tile = World::TileManager::get(World::Pos2{ bogie.tileX, bogie.tileY });
        for (auto& el : tile)
        {
            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }

            if (elTrack->baseZ() != bogie.tileBaseZ)
            {
                continue;
            }

            if (elTrack->rotation() != bogie.trackAndDirection.track.cardinalDirection())
            {
                continue;
            }

            if (elTrack->trackId() != bogie.trackAndDirection.track.id())
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

    // 0x004AA50
    static bool isOnRackRailRoad(const VehicleBogie& bogie)
    {
        if (bogie.trackType == 0xFFU)
        {
            return true;
        }
        auto* roadObj = ObjectManager::get<RoadObject>(bogie.trackType);
        if (!roadObj->hasFlags(RoadObjectFlags::unk_05))
        {
            return true;
        }

        auto* vehObj = ObjectManager::get<VehicleObject>(bogie.objectId);
        if (!vehObj->hasFlags(VehicleObjectFlags::rackRail))
        {
            return false;
        }

        const auto tile = World::TileManager::get(World::Pos2{ bogie.tileX, bogie.tileY });
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }

            if (elRoad->baseZ() != bogie.tileBaseZ)
            {
                continue;
            }

            if (elRoad->rotation() != bogie.trackAndDirection.road.cardinalDirection())
            {
                continue;
            }

            if (elRoad->roadId() != bogie.trackAndDirection.road.id())
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

    // 0x004AA97A
    bool VehicleBogie::isOnRackRail()
    {
        if (mode == TransportMode::rail)
        {
            return isOnRackRailRail(*this);
        }
        else if (mode == TransportMode::road)
        {
            return isOnRackRailRoad(*this);
        }
        else
        {
            assert(false);
            return false;
        }
    }

    // 0x004BA873
    // esi : vehBogie
    void sub_4BA873(VehicleBogie& vehBogie)
    {
        vehBogie.timeoutToBreakdown = 0xFFFF;
        if (vehBogie.reliability != 0)
        {
            int32_t reliabilityFactor = vehBogie.reliability / 256;
            reliabilityFactor *= reliabilityFactor;
            reliabilityFactor /= 16;

            auto& prng = gPrng1();
            int32_t randVal = (prng.randNext(65535) * (reliabilityFactor / 2)) / 65536;
            reliabilityFactor -= reliabilityFactor / 4;
            reliabilityFactor += randVal;
            vehBogie.timeoutToBreakdown = static_cast<uint16_t>(std::max(4, reliabilityFactor));
        }
    }
}
