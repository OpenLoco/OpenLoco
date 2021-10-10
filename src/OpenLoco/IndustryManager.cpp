#include "IndustryManager.h"
#include "CompanyManager.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "Math/Vector.hpp"
#include "Objects/IndustryObject.h"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::IndustryManager
{
    static auto& rawIndustries() { return getGameState().industries; }

    // 0x00453214
    void reset()
    {
        for (auto& industry : rawIndustries())
        {
            industry.name = StringIds::null;
        }
        Ui::Windows::IndustryList::reset();
    }

    FixedVector<Industry, Limits::maxIndustries> industries()
    {
        return FixedVector(rawIndustries());
    }

    Industry* get(IndustryId id)
    {
        if (enumValue(id) >= Limits::maxIndustries)
        {
            return nullptr;
        }
        return &rawIndustries()[enumValue(id)];
    }

    // 0x00453234
    void update()
    {
        if ((getGameState().flags & (1u << 0)) != 0u && !isEditorMode())
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
        if ((getGameState().flags & (1u << 0)) == 0u)
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

OpenLoco::IndustryId OpenLoco::Industry::id() const
{
    auto* first = &IndustryManager::rawIndustries()[0];
    return IndustryId(this - first);
}
