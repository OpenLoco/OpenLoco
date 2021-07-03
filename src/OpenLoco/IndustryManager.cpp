#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Interop/Interop.hpp"
#include "Math/Vector.hpp"
#include "Objects/IndustryObject.h"
#include "OpenLoco.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::IndustryManager
{
    static loco_global<Industry[max_industries], 0x005C455C> _industries;

    // 0x00453214
    void reset()
    {
        call(0x00453214);
    }

    LocoFixedVector<Industry> industries()
    {
        return LocoFixedVector<Industry>(_industries);
    }

    Industry* get(IndustryId_t id)
    {
        if (id >= _industries.size())
        {
            return nullptr;
        }
        return &_industries[id];
    }

    // 0x00453234
    void update()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !isEditorMode())
        {
            CompanyManager::updatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.update();
            }
        }
    }

    // 0x0045383B
    void updateMonthly()
    {
        call(0x0045383B);
    }

    // 0x00459D2D
    void createAllMapAnimations()
    {
        if (!(addr<0x00525E28, uint32_t>() & (1 << 0)))
            return;

        for (auto& industry : industries())
        {
            industry.createMapAnimations();
        }
    }

    // 0x048FE92
    bool industryNearPosition(const Map::Pos2& position, uint32_t flags)
    {
        for (auto& industry : industries())
        {
            auto industryObj = industry.object();
            if ((industryObj->flags & flags) == 0)
                continue;

            auto manhattanDistance = Math::Vector::manhattanDistance(Map::Pos2{ industry.x, industry.y }, position);
            if (manhattanDistance / Map::tile_size < 11)
                return true;
        }

        return false;
    }
}
