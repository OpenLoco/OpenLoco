#include "Effect.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "MoneyEffect.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x004405CD
    void EffectEntity::update()
    {
        switch (getSubType())
        {
            case EffectType::exhaust:
                asExhaust()->update();
                break;
            case EffectType::redGreenCurrency:
                asRedGreenCurrency()->update();
                break;
            case EffectType::windowCurrency:
                asWindowCurrency()->update();
                break;
            case EffectType::vehicleCrashParticle:
                asVehicleCrashParticle()->update();
                break;
            case EffectType::explosionCloud:
                asExplosionCloud()->update();
                break;
            case EffectType::splash:
                asSplash()->update();
                break;
            case EffectType::fireball:
                asFireball()->update();
                break;
            case EffectType::explosionSmoke:
                asExplosionSmoke()->update();
                break;
            case EffectType::smoke:
                asSmoke()->update();
                break;
        }
    }

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
            if (el.isFlag5() || el.isGhost())
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
            if (elStation->isFlag5() || elStation->isGhost())
            {
                continue;
            }

            const auto* stationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
            if (stationObj->hasFlags(TrainStationFlags::unk1))
            {
                continue;
            }

            if (elStation->multiTileIndex() == 0)
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
        if (!World::validCoords(loc))
            return nullptr;
        auto surface = World::TileManager::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

        if (surface == nullptr)
            return nullptr;

        if (loc.z <= surface->baseHeight())
            return nullptr;

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

    // 0x004407A1
    void Smoke::update()
    {
        Ui::ViewportManager::invalidate(this, ZoomLevel::half);
        moveTo(position + World::Pos3(0, 0, 1));
        Ui::ViewportManager::invalidate(this, ZoomLevel::half);

        frame += 0x55;
        if (frame >= 0xC00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x00440BEB
    Smoke* Smoke::create(World::Pos3 loc)
    {
        auto t = static_cast<Smoke*>(EntityManager::createEntityMisc());
        if (t != nullptr)
        {
            t->spriteWidth = 44;
            t->spriteHeightNegative = 32;
            t->spriteHeightPositive = 34;
            t->baseType = EntityBaseType::effect;
            t->moveTo(loc);
            t->setSubType(EffectType::smoke);
            t->frame = 0;
        }
        return t;
    }

    // 0x004406A0
    void VehicleCrashParticle::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004406A0, regs);
    }

    // 0x004407CC
    void ExplosionCloud::update()
    {
        invalidateSprite();
        frame += 0x80;
        if (frame >= 0x1200)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x004407E0
    void Splash::update()
    {
        invalidateSprite();
        frame += 0x55;
        if (frame >= 0x1C00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x004407F3
    void Fireball::update()
    {
        invalidateSprite();
        frame += 0x40;
        if (frame >= 0x1F00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x00440078D
    void ExplosionSmoke::update()
    {
        invalidateSprite();
        frame += 0x80;
        if (frame >= 0xA00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x00440BBF
    ExplosionSmoke* ExplosionSmoke::create(const World::Pos3& loc)
    {
        auto t = static_cast<ExplosionSmoke*>(EntityManager::createEntityMisc());
        if (t != nullptr)
        {
            t->spriteWidth = 44;
            t->spriteHeightNegative = 32;
            t->spriteHeightPositive = 34;
            t->baseType = EntityBaseType::effect;
            t->moveTo(loc + World::Pos3{ 0, 0, 4 });
            t->setSubType(EffectType::explosionSmoke);
            t->frame = 0;
        }
        return t;
    }
}
