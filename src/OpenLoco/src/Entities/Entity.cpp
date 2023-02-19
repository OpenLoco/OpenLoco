#include "Entity.h"
#include "Config.h"
#include "EntityManager.h"
#include "Graphics/Gfx.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>

using namespace OpenLoco;
using namespace OpenLoco::Interop;

bool EntityBase::empty() const
{
    return baseType == EntityBaseType::null;
}

// 0x0046FC83
void EntityBase::moveTo(const Map::Pos3& loc)
{
    EntityManager::moveSpatialEntry(*this, loc);

    // Update sprite boundaries
    if (position.x == Location::null)
    {
        spriteLeft = Location::null;
        return;
    }

    const auto vpPos = Map::gameToScreen(loc, Ui::WindowManager::getCurrentRotation());
    spriteLeft = vpPos.x - spriteWidth;
    spriteRight = vpPos.x + spriteWidth;
    spriteTop = vpPos.y - spriteHeightNegative;
    spriteBottom = vpPos.y + spriteHeightPositive;
}

// 0x004CBB01
void OpenLoco::EntityBase::invalidateSprite()
{
    Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
}
