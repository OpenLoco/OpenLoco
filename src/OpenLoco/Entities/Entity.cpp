#include "Entity.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "EntityManager.h"
#include <algorithm>

using namespace OpenLoco;
using namespace OpenLoco::Interop;

bool EntityBase::empty() const
{
    return base_type == EntityBaseType::null;
}

// 0x0046FC83
void EntityBase::moveTo(const Map::Pos3& loc)
{
    EntityManager::moveSpatialEntry(*this, loc);
    // x and y were already updated in moveSpatialEntry
    position.z = loc.z;

    // Update sprite boundaries

    if (position.x == Location::null)
    {
        sprite_left = Location::null;
        return;
    }

    const auto vpPos = Map::gameToScreen(loc, Ui::WindowManager::getCurrentRotation());
    sprite_left = vpPos.x - var_14;
    sprite_right = vpPos.x + var_14;
    sprite_top = vpPos.y - var_09;
    sprite_bottom = vpPos.y + var_15;
}

// 0x004CBB01
void OpenLoco::EntityBase::invalidateSprite()
{
    Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
}
