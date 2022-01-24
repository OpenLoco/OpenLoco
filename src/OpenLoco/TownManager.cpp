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

    FixedVector<Town, Limits::maxTowns> towns()
    {
        return FixedVector(rawTowns());
    }

    Town* get(TownId id)
    {
        if (enumValue(id) >= Limits::maxTowns)
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
                    CompanyManager::updatingCompanyId(CompanyId::neutral);
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
            // Scroll history
            if (currTown.historySize == std::size(currTown.history))
            {
                for (size_t i = 0; i < std::size(currTown.history) - 1; i++)
                    currTown.history[i] = currTown.history[i + 1];
            }
            else
                currTown.historySize++;

            // Compute population growth.
            uint32_t popSteps = std::max<int32_t>(currTown.population - currTown.history_min_population, 0) / 50;
            uint32_t popGrowth = 0;
            while (popSteps > 255)
            {
                popSteps -= 20;
                popGrowth += 1000;
            }

            // Any population growth to account for?
            if (popGrowth != 0)
            {
                currTown.history_min_population += popGrowth;

                uint8_t offset = (popGrowth / 50) & 0xFF;
                for (uint8_t i = 0; i < currTown.historySize; i++)
                {
                    int16_t newHistory = currTown.history[i] - offset;
                    currTown.history[i] = newHistory >= 0 ? static_cast<uint8_t>(newHistory) : 0;
                }
            }

            // Write new history point.
            auto histIndex = std::clamp<int32_t>(currTown.historySize - 1, 0, std::size(currTown.history));
            currTown.history[histIndex] = popSteps & 0xFF;

            // Find historical maximum population.
            uint8_t maxPopulation = 0;
            for (int i = 0; i < currTown.historySize; i++)
                maxPopulation = std::max(maxPopulation, currTown.history[i]);

            int32_t popOffset = currTown.history_min_population;
            while (maxPopulation <= 235 && popOffset > 0)
            {
                maxPopulation += 20;
                popOffset -= 1000;
            }

            popOffset -= currTown.history_min_population;
            if (popOffset != 0)
            {
                popOffset = -popOffset;
                currTown.history_min_population -= popOffset;
                popOffset /= 50;

                for (int i = 0; i < currTown.historySize; i++)
                    currTown.history[i] += popOffset;
            }

            // Work towards computing new build speed.
            // will be the smallest of the influence cargo delivered to the town
            // i.e. to get maximum growth max of the influence cargo must be delivered
            // every update. If no influence cargo the grows at max rate
            uint16_t minCargoDelivered = std::numeric_limits<uint16_t>::max();
            uint32_t cargoFlags = currTown.cargo_influence_flags;
            while (cargoFlags != 0)
            {
                uint32_t cargoId = Utility::bitScanForward(cargoFlags);
                cargoFlags &= ~(1 << cargoId);

                minCargoDelivered = std::min(minCargoDelivered, currTown.monthly_cargo_delivered[cargoId]);
            }

            // Compute build speed (1=slow build speed, 4=fast build speed)
            currTown.build_speed = std::clamp((minCargoDelivered / 100) + 1, 1, 4);

            // Reset all monthly_cargo_delivered intermediaries to zero.
            memset(&currTown.monthly_cargo_delivered, 0, sizeof(currTown.monthly_cargo_delivered));
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
    if (index > Limits::maxTowns)
    {
        return OpenLoco::TownId::null;
    }
    return OpenLoco::TownId(index);
}
