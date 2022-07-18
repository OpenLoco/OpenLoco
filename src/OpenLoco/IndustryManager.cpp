#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Game.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "Math/Vector.hpp"
#include "Objects/IndustryObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::IndustryManager
{
    static auto& rawIndustries() { return getGameState().industries; }
    uint8_t getFlags() { return getGameState().industryFlags; }

    void setFlags(const uint8_t flags)
    {
        getGameState().industryFlags = flags;
    }

    // 0x00453214
    void reset()
    {
        for (auto& industry : rawIndustries())
        {
            industry.name = StringIds::null;
        }
        Ui::Windows::IndustryList::reset();
    }

    FixedVector<Industry, Limits::kMaxIndustries> industries()
    {
        return FixedVector(rawIndustries());
    }

    Industry* get(IndustryId id)
    {
        if (enumValue(id) >= Limits::kMaxIndustries)
        {
            return nullptr;
        }
        return &rawIndustries()[enumValue(id)];
    }

    // 0x00453234
    void update()
    {
        if (Game::hasFlags(1u << 0) && !isEditorMode())
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.update();
            }
        }
    }

    // 0x00453487
    void updateDaily()
    {
        if (Game::hasFlags(1u << 0))
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.updateDaily();
            }
        }
    }

    // 0x00459659
    static void tryCreateNewIndustriesMonthly()
    {
        call(0x00459659);
    }

    // 0x0045383B
    void updateMonthly()
    {
        if (Game::hasFlags(1u << 0))
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            tryCreateNewIndustriesMonthly();

            for (auto& industry : industries())
            {
                industry.updateMonthly();
            }

            Ui::WindowManager::invalidate(Ui::WindowType::industry);
        }
    }

    // 0x00459D2D
    void createAllMapAnimations()
    {
        if (!Game::hasFlags(1u << 0))
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
            const auto* industryObj = industry.getObject();
            if ((industryObj->flags & flags) == 0)
                continue;

            auto manhattanDistance = Math::Vector::manhattanDistance(Map::Pos2{ industry.x, industry.y }, position);
            if (manhattanDistance / Map::tile_size < 11)
                return true;
        }

        return false;
    }

    // 0x004574E8
    void updateProducedCargoStats()
    {
        for (auto& industry : industries())
        {
            industry.updateProducedCargoStats();
        }
    }
}

OpenLoco::IndustryId OpenLoco::Industry::id() const
{
    auto* first = &IndustryManager::rawIndustries()[0];
    return IndustryId(this - first);
}
