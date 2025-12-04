#include "VehicleBogie.h"
#include "Effects/ExplosionEffect.h"
#include "Effects/SplashEffect.h"
#include "Effects/VehicleCrashEffect.h"
#include "Entities/EntityManager.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "VehicleBody.h"
#include "VehicleHead.h"

#include <OpenLoco/Math/Trigonometry.hpp>
#include <cstdint>

using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
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
        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }

        auto& distances = getVehicleUpdateDistances();
        const auto motionResult = updateTrackMotion(distances.unkDistance1, false);

        int32_t unkDistance = distances.unkDistance2;
        if (wheelSlipping != 0)
        {
            auto unk = wheelSlipping;
            if (unk > kWheelSlippingDuration / 2)
            {
                unk = kWheelSlippingDuration - unk;
            }
            unkDistance = 500 + unk * 320;
        }

        updateRoll(unkDistance);
        if (motionResult.hasFlags(UpdateVar1136114Flags::noRouteFound))
        {
            destroyTrain();
            return false;
        }
        else if (!motionResult.hasFlags(UpdateVar1136114Flags::crashed))
        {
            return true;
        }

        collision(motionResult.collidedEntityId);
        return false;
    }

    // 0x004AAC02
    void VehicleBogie::updateRoll(const int32_t unkDistance)
    {
        auto unk = unkDistance / 8;
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

    // To replace several instances of the same exact code in
    // VehicleBogie::updateSegmentCrashed(). Returns true if the
    // vehicle crashed in the process.
    static bool rotateAndExplodeIfNotAlreadyExploded(VehicleBogie& bogie, bool direction)
    {
        const auto slightlyRotated = direction ? 4 : -4;
        bogie.spriteYaw = (bogie.spriteYaw + slightlyRotated) & 0x3F;

        // explode, if we haven't already
        if (!bogie.hasVehicleFlags(VehicleFlags::unk_5))
        {
            bogie.explodeComponent();
            bogie.vehicleFlags |= VehicleFlags::unk_5;
            return true;
        }
        else
        {
            return false;
        }
    }

    // 0x00462893
    static bool checkForTileCollision(const World::Pos3 pos)
    {
        const auto xNibble = pos.x & 0x1F;
        const auto yNibble = pos.y & 0x1F;
        uint8_t occupiedQuarter = 0U;
        if (xNibble < 16)
        {
            occupiedQuarter = 1U << 2;
            if (yNibble >= 16)
            {
                occupiedQuarter = 1U << 3;
            }
        }
        else
        {
            occupiedQuarter = 1U << 0;
            if (yNibble < 16)
            {
                occupiedQuarter = 1U << 1;
            }
        }

        const auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            if (pos.z < el.baseHeight())
            {
                continue;
            }
            if (pos.z >= el.clearHeight())
            {
                continue;
            }
            if (el.occupiedQuarter() & occupiedQuarter)
            {
                return true;
            }
        }
        return false;
    }

    // 0x0004AA959
    static bool destroyedBogieCheckForCollision(VehicleBogie& bogie, World::Pos3& pos)
    {
        if (checkForTileCollision(pos))
        {
            return true;
        }
        return checkForCollisions(bogie, pos) != EntityId::null;
    }

    // 0x004AA68E
    void VehicleBogie::updateSegmentCrashed()
    {
        Speed32 speed = Speed32(var_5A & 0x7FFFFFFF);
        bool isComponentDestroyed = this->var_5A & (1U << 31);
        speed = speed - (speed / 64);
        if (speed <= 2.0_mph)
        {
            speed = 0.0_mph;
        }

        this->var_5A = speed.getRaw() | (isComponentDestroyed ? (1U << 31) : 0);

        auto& distances = getVehicleUpdateDistances();
        distances.unkDistance1 = speed.getRaw() / 128;
        distances.unkDistance2 = speed.getRaw() / 128;

        this->updateRoll(distances.unkDistance1);

        if (isComponentDestroyed)
        {
            if (speed >= 1.0_mph)
            {
                const auto distanceTravelled = speed.getRaw() / 16;
                auto xyDistance = Math::Trigonometry::computeXYVector(distanceTravelled, spriteYaw);

                uint16_t xDistanceLow = xyDistance.x & 0x0000FFFF;
                uint16_t yDistanceLow = xyDistance.y & 0x0000FFFF;
                // This is being casted to uint32_t as we want a negative value to round away from zero
                // this is due to splitting the precision of the distance into two parts
                World::Pos2 distanceWorld(static_cast<uint32_t>(xyDistance.x) / 65536, static_cast<uint32_t>(xyDistance.y) / 65536);
                // We are storing the lower precision in tileX and tileY they aren't actually being used as tileX and tileY.
                // Yay reusing fields for different purposes means we need to be careful with the sign of the type
                int32_t newTileX = static_cast<uint16_t>(this->tileX) + xDistanceLow;
                if (newTileX >= 0x00010000)
                {
                    distanceWorld.x++;
                }
                this->tileX = newTileX & 0x0000FFFF;

                int32_t newTileY = static_cast<uint16_t>(this->tileY) + yDistanceLow;
                if (newTileY >= 0x00010000)
                {
                    distanceWorld.y++;
                }
                this->tileY = newTileY & 0x0000FFFF;

                this->tileBaseZ++;
                int16_t zDistance = this->tileBaseZ / 32;

                // Calculate new position - but don't update yet!! This is pushed to the stack.
                World::Pos3 newPosition{ position + World::Pos3(distanceWorld, -zDistance) };

                if (!destroyedBogieCheckForCollision(*this, position))
                {
                    World::Pos3 positionToTest = { newPosition.x, newPosition.y, this->position.z };
                    if (destroyedBogieCheckForCollision(*this, positionToTest))
                    {
                        newPosition.x = this->position.x;
                        newPosition.y = this->position.y;

                        if (speed >= 5.0_mph)
                        {
                            rotateAndExplodeIfNotAlreadyExploded(*this, true);
                        }

                        this->var_5A = ((speed / 2).getRaw() | (1U << 31));
                    }

                    if (destroyedBogieCheckForCollision(*this, newPosition))
                    {
                        newPosition.z = this->position.z;
                        if (this->tileBaseZ >= 0x0A)
                        {
                            rotateAndExplodeIfNotAlreadyExploded(*this, false);
                        }

                        this->tileBaseZ = 0;
                    }
                }

                const auto newTileHeight = World::TileManager::getHeight(newPosition);
                if (newTileHeight.landHeight >= this->position.z
                    || newTileHeight.landHeight >= newPosition.z)
                {
                    if (newTileHeight.landHeight < this->position.z
                        && newTileHeight.landHeight >= newPosition.z)
                    {
                        newPosition.z = this->position.z;
                        this->tileBaseZ = 0;
                    }

                    newPosition.x = this->position.x;
                    newPosition.y = this->position.y;

                    if (speed >= 5.0_mph)
                    {
                        rotateAndExplodeIfNotAlreadyExploded(*this, true);
                    }

                    this->var_5A = ((speed / 2).getRaw() | (1U << 31));
                }

                if (newTileHeight.waterHeight != 0
                    && newTileHeight.waterHeight < this->position.z
                    && newTileHeight.waterHeight >= newPosition.z)
                {
                    this->spriteYaw = (this->spriteYaw + 4) & 0x3F;
                    if (!this->hasVehicleFlags(VehicleFlags::unk_5))
                    {
                        World::Pos3 splashPos{ this->position.x, this->position.y, newTileHeight.waterHeight };
                        Splash::create(splashPos);
                        Audio::playSound(Audio::SoundId::splash2, splashPos);
                        this->vehicleFlags |= VehicleFlags::unk_5;
                    }
                }

                this->moveTo(newPosition);
                this->invalidateSprite();
            }
        }
        else
        {
            if (this->mode != TransportMode::road)
            {
                const auto motionResult = this->updateTrackMotion(distances.unkDistance1, false);
                if (motionResult.hasFlags(UpdateVar1136114Flags::unk_m00 | UpdateVar1136114Flags::noRouteFound))
                {
                    this->var_5A |= 1U << 31;
                }
            }
        }
    }

    // 0x004AA0DF
    void VehicleBogie::collision(const EntityId collideEntityId)
    {
        destroyTrain();
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
        auto* collideEntity = EntityManager::get<EntityBase>(collideEntityId);
        auto* collideCarComponent = collideEntity->asBase<VehicleBase>();
        if (collideCarComponent != nullptr)
        {
            Vehicle collideTrain(collideCarComponent->getHead());
            if (collideTrain.head->status != Status::crashed)
            {
                collideCarComponent->destroyTrain();
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
