#include "Effect.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "ExhaustEffect.h"
#include "ExplosionEffect.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "MoneyEffect.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "SceneManager.h"
#include "SplashEffect.h"
#include "Ui/WindowManager.h"
#include "VehicleCrashEffect.h"
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
