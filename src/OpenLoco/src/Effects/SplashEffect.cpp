#include "SplashEffect.h"
#include "Entities/EntityManager.h"

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
}
