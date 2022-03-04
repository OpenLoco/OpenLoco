#include "TownManager.h"
#include "CompanyManager.h"
#include "Game.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"
#include "Utility/Numeric.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::TownManager
{
    static loco_global<Town*, 0x01135C38> dword_1135C38;

    // 0x00497DC1
    // The return value of this function is also being returned via dword_1135C38.
    Town* sub_497DC1(const Map::Pos2& loc, uint32_t population, uint32_t unk1, int16_t rating, uint16_t unk3)
    {
        auto res = getClosestTownAndUnk(loc);
        if (res == std::nullopt)
        {
            dword_1135C38 = nullptr;
            return nullptr;
        }
        auto townId = res->first;
        auto town = get(townId);
        dword_1135C38 = town;
        if (town != nullptr)
        {
            town->var_34 += unk1;
        }
        if (population != 0)
        {
            town->population += population;
            Ui::WindowManager::invalidate(Ui::WindowType::townList);
            Ui::WindowManager::invalidate(Ui::WindowType::town, enumValue(town->id()));
        }
        if (rating != 0)
        {
            auto companyId = CompanyManager::getUpdatingCompanyId();
            if (companyId != CompanyId::neutral)
            {
                if (!isEditorMode())
                {
                    town->adjustCompanyRating(companyId, rating);
                    Ui::WindowManager::invalidate(Ui::WindowType::town, enumValue(town->id()));
                }
            }
        }

        if (town->var_38 + unk3 <= std::numeric_limits<uint16_t>::max())
        {
            town->var_38 += unk3;
        }

        return town;
    }

    static auto& rawTowns() { return getGameState().towns; }

    // 0x00496B38
    void reset()
    {
        for (auto& town : rawTowns())
        {
            town.name = StringIds::null;
        }
        Ui::Windows::TownList::reset();
    }

    FixedVector<Town, Limits::kMaxTowns> towns()
    {
        return FixedVector(rawTowns());
    }

    Town* get(TownId id)
    {
        if (enumValue(id) >= Limits::kMaxTowns)
        {
            return nullptr;
        }
        return &rawTowns()[enumValue(id)];
    }

    // 0x00496B6D
    void update()
    {
        if (Game::hasFlags(1u << 0) && !isEditorMode())
        {
            auto ticks = scenarioTicks();
            if (ticks % 8 == 0)
            {
                const auto id = TownId((ticks / 8) % 0x7F);
                auto town = get(id);
                if (town != nullptr && !town->empty())
                {
                    CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
                    town->update();
                }
            }
        }
    }

    // 0x0049771C
    void updateLabels()
    {
        for (Town& town : towns())
        {
            town.updateLabel();
        }
    }

    // 0x0049748C
    void updateMonthly()
    {
        for (Town& currTown : towns())
        {
            currTown.updateMonthly();
        }

        Ui::WindowManager::invalidate(Ui::WindowType::town);
    }

    // 0x00497E52
    std::optional<std::pair<TownId, uint8_t>> getClosestTownAndUnk(const Map::Pos2& loc)
    {
        int32_t closestDistance = std::numeric_limits<uint16_t>::max();
        auto closestTown = TownId::null; // ebx
        for (const auto& town : towns())
        {
            const auto distance = Math::Vector::manhattanDistance(Map::Pos2(town.x, town.y), loc);
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestTown = town.id();
            }
        }

        if (closestDistance == std::numeric_limits<uint16_t>::max())
        {
            return std::nullopt;
        }

        const auto* town = get(closestTown);
        if (town == nullptr)
        {
            return std::nullopt;
        }
        const int32_t realDistance = Math::Vector::distance(Map::Pos2(town->x, town->y), loc);
        const auto unk = std::clamp((realDistance - town->var_38 * 4 + 512) / 128, 0, 4);
        const uint8_t invUnk = std::min(4 - unk, 3); //edx
        return { std::make_pair(town->id(), invUnk) };
    }

}

OpenLoco::TownId OpenLoco::Town::id() const
{
    // TODO check if this is stored in Town structure
    //      otherwise add it when possible
    auto index = static_cast<size_t>(this - &TownManager::rawTowns()[0]);
    if (index > Limits::kMaxTowns)
    {
        return OpenLoco::TownId::null;
    }
    return OpenLoco::TownId(index);
}
