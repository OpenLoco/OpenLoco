#include "SurfaceElement.h"
#include "TileManager.h"
#include "TownManager.h"
#include "ViewportManager.h"

namespace OpenLoco::World
{
    void SurfaceElement::removeIndustry(const World::Pos2& pos)
    {
        if (isIndustrial())
        {
            setIsIndustrialFlag(false);
            setVar6SLR5(0);
            setIndustry(IndustryId(0));
            auto z = baseHeight();
            Ui::ViewportManager::invalidate(pos, z, z + 32, ZoomLevel::eighth);
            TownManager::sub_497DC1(pos, 0, 0, -30, 0);
        }
    }
}
