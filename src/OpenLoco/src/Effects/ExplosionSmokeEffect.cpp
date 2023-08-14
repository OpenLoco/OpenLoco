#include "ExplosionSmokeEffect.h"
#include "Entities/EntityManager.h"

namespace OpenLoco
{
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
