#include "Entity.h"
#include "EntityManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

namespace OpenLoco
{
    bool EntityBase::empty() const
    {
        return baseType == EntityBaseType::null;
    }

    // 0x0046FC83
    void EntityBase::moveTo(const World::Pos3& loc)
    {
        if (position == loc)
        {
            // No-op.
            return;
        }

        // Pre invalidation.
        if (position.x != Location::null)
        {
            invalidateSprite();
        }

        EntityManager::moveSpatialEntry(*this, loc);

        // Update position
        position = loc;

        // Update sprite boundaries
        if (position.x == Location::null)
        {
            spriteLeft = Location::null;
            return;
        }

        const auto vpPos = World::gameToScreen(loc, Ui::WindowManager::getCurrentRotation());
        spriteLeft = vpPos.x - spriteWidth;
        spriteRight = vpPos.x + spriteWidth;
        spriteTop = vpPos.y - spriteHeightNegative;
        spriteBottom = vpPos.y + spriteHeightPositive;

        // Post invalidation.
        invalidateSprite();
    }

    // 0x004CBB01
    void EntityBase::invalidateSprite()
    {
        Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
    }
}
