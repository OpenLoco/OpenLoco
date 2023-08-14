#include "SmokeEffect.h"
#include "Entities/EntityManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

namespace OpenLoco
{
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
}
