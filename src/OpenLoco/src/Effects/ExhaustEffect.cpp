#include "ExhaustEffect.h"
#include "Entities/EntityManager.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/SteamObject.h"
#include "Objects/TrainStationObject.h"

namespace OpenLoco
{
    const SteamObject* Exhaust::getObject() const
    {
        return ObjectManager::get<SteamObject>(objectId & 0x7F);
    }

    // 0x004408C2
    void Exhaust::update()
    {
        const auto* steamObj = getObject();
        if (steamObj->hasFlags(SteamObjectFlags::applyWind))
        {
            // Wind is applied by applying a slight modification (~1 in 10 ticks a pixel change) to the x component of the exhaust
            const auto res = windProgress + 7000;
            windProgress = static_cast<uint16_t>(res);
            auto newPos = position;
            newPos.x += res / (std::numeric_limits<uint16_t>::max() + 1);
            if (newPos.x != position.x)
            {
                invalidateSprite();
                moveTo(newPos);
                invalidateSprite();
            }
        }
        stationaryProgress++;
        if (stationaryProgress < steamObj->numStationaryTicks)
        {
            return;
        }
        stationaryProgress = 0;
        frameNum++;

        auto [totalNumFrames, frameInfo] = steamObj->getFramesInfo(isSubObjType1());

        if (frameNum >= totalNumFrames)
        {
            invalidateSprite();
            EntityManager::freeEntity(this);
            return;
        }

        auto newPos = position;
        newPos.z += frameInfo[frameNum].height;
        invalidateSprite();
        moveTo(newPos);
        invalidateSprite();

        if (!steamObj->hasFlags(SteamObjectFlags::disperseOnCollision))
        {
            return;
        }

        const auto tile = World::TileManager::get(position);
        const auto lowZ = (position.z / World::kSmallZStep) - 3;
        const auto highZ = lowZ + 6;
        for (const auto& el : tile)
        {
            if (el.isAiAllocated() || el.isGhost())
            {
                continue;
            }
            if (lowZ < el.baseZ() && highZ > el.baseZ())
            {
                invalidateSprite();
                EntityManager::freeEntity(this);
                return;
            }

            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (!elTrack->hasStationElement())
            {
                continue;
            }
            auto* elStation = elTrack->next()->as<World::StationElement>();
            if (elStation == nullptr)
            {
                continue;
            }
            if (elStation->isAiAllocated() || elStation->isGhost())
            {
                continue;
            }

            const auto* stationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
            if (stationObj->hasFlags(TrainStationFlags::noCanopy))
            {
                continue;
            }

            if (elStation->sequenceIndex() == 0)
            {
                continue;
            }

            const auto cmpZ = elTrack->baseZ() + 7;
            if (lowZ >= cmpZ)
            {
                continue;
            }
            if (highZ <= cmpZ)
            {
                continue;
            }

            invalidateSprite();
            EntityManager::freeEntity(this);
            return;
        }
    }

    // 0x0044080C
    Exhaust* Exhaust::create(World::Pos3 loc, uint8_t type)
    {
        if (!World::TileManager::validCoords(loc))
        {
            return nullptr;
        }
        auto surface = World::TileManager::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

        if (surface == nullptr)
        {
            return nullptr;
        }

        if (loc.z <= surface->baseHeight())
        {
            return nullptr;
        }

        auto _exhaust = static_cast<Exhaust*>(EntityManager::createEntityMisc());

        if (_exhaust != nullptr)
        {
            _exhaust->baseType = EntityBaseType::effect;
            _exhaust->moveTo(loc);
            _exhaust->objectId = type;
            const auto* obj = _exhaust->getObject();
            _exhaust->spriteWidth = obj->spriteWidth;
            _exhaust->spriteHeightNegative = obj->spriteHeightNegative;
            _exhaust->spriteHeightPositive = obj->spriteHeightPositive;
            _exhaust->setSubType(EffectType::exhaust);
            _exhaust->frameNum = 0;
            _exhaust->stationaryProgress = 0;
            _exhaust->windProgress = 0;
            _exhaust->var_34 = 0;
            _exhaust->var_36 = 0;
        }
        return _exhaust;
    }
}
