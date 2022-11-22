#include "Entity.h"
#include "Config.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "EntityManager.h"
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
    spriteLeft = vpPos.x - var_14;
    spriteRight = vpPos.x + var_14;
    spriteTop = vpPos.y - var_09;
    spriteBottom = vpPos.y + var_15;
}

// 0x004CBB01
void OpenLoco::EntityBase::invalidateSprite()
{
    Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
}
