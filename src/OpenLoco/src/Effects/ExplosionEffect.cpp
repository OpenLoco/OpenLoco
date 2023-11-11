#include "ExplosionEffect.h"
#include "Entities/EntityManager.h"

#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
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

    // 0x00440C13
    ExplosionCloud* ExplosionCloud::create(const World::Pos3& loc)
    {
        auto* e = static_cast<ExplosionCloud*>(EntityManager::createEntityMisc());
        if (e != nullptr)
        {
            e->spriteWidth = 44;
            e->spriteHeightNegative = 32;
            e->spriteHeightPositive = 34;
            e->baseType = EntityBaseType::effect;
            e->moveTo(loc);
            e->setSubType(EffectType::explosionCloud);
            e->frame = 0;
        }
        return e;
    }
}
