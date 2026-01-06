#include "SplashEffect.h"
#include "Entities/EntityManager.h"
#include "Map/TileManager.h"

namespace OpenLoco
{
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

    // 0x00440C6B
    Splash* Splash::create(const World::Pos3& pos)
    {
        if (!World::TileManager::validCoords(pos))
        {
            return nullptr;
        }

        auto* splashEnt = static_cast<Splash*>(EntityManager::createEntityMisc());
        if (splashEnt == nullptr)
        {
            return nullptr;
        }

        splashEnt->baseType = EntityBaseType::effect;
        splashEnt->spriteWidth = 33;
        splashEnt->spriteHeightNegative = 51;
        splashEnt->spriteHeightPositive = 16;
        splashEnt->setSubType(EffectType::splash);
        splashEnt->moveTo(pos + World::Pos3{ 0, 3, 0 });
        splashEnt->frame = 0;

        return splashEnt;
    }

}
