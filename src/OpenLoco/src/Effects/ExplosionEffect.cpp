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
}
