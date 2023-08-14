#include "FireballEffect.h"
#include "Entities/EntityManager.h"

namespace OpenLoco
{
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
}
