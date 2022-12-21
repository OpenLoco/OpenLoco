#include "TownManager.h"
#include "CompanyManager.h"
#include "Game.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Map/BuildingElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/Numeric.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::TownManager
{
    static loco_global<Town*, 0x01135C38> _dword_1135C38;

    // 0x00497DC1
    // The return value of this function is also being returned via dword_1135C38.
    // esi population
    // edi capacity
    // ebp rating | (numBuildings << 16)
    Town* updateTownInfo(const World::Pos2& loc, uint32_t population, uint32_t populationCapacity, int16_t rating, int16_t numBuildings)
    {
        auto res = getClosestTownAndDensity(loc);
        if (res == std::nullopt)
        {
            _dword_1135C38 = nullptr;
            return nullptr;
        }
        auto townId = res->first;
        auto town = get(townId);
        _dword_1135C38 = town;
        if (town != nullptr)
        {
            town->populationCapacity += populationCapacity;
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

        if (town->numBuildings + numBuildings <= std::numeric_limits<int16_t>::max())
        {
            town->numBuildings += numBuildings;
        }

        return town;
    }

    static auto& rawTowns() { return getGameState().towns; }

    // 0x00497348
    void resetBuildingsInfluence()
    {
        for (auto& town : towns())
        {
            town.numBuildings = 0;
            town.population = 0;
            town.populationCapacity = 0;
            std::fill(std::begin(town.var_150), std::end(town.var_150), 0);
        }

        World::TilePosRangeView tileLoop{ { 1, 1 }, { World::kMapColumns - 1, World::kMapRows - 1 } };
        for (const auto& tilePos : tileLoop)
        {
            auto tile = World::TileManager::get(tilePos);
            for (auto& element : tile)
            {
                auto* building = element.as<World::BuildingElement>();
                if (building == nullptr)
                    continue;

                if (building->isGhost())
                    continue;

                if (building->has_40())
                    continue;

                if (building->multiTileIndex() != 0)
                    continue;

                auto objectId = building->objectId();
                auto* buildingObj = ObjectManager::get<BuildingObject>(objectId);
                auto producedQuantity = buildingObj->producedQuantity[0];
                uint32_t population;
                if (!building->isConstructed())
                {
                    population = 0;
                }
                else
                {
                    population = producedQuantity;
                }
                auto* town = updateTownInfo(tilePos, population, producedQuantity, 0, 1);
                if (town != nullptr)
                {
                    if (buildingObj->var_AC != 0xFF)
                    {
                        town->var_150[buildingObj->var_AC] += 1;
                    }
                }
            }
        }
        Gfx::invalidateScreen();
    }

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
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
        {
            auto ticks = ScenarioManager::getScenarioTicks();
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
    std::optional<std::pair<TownId, uint8_t>> getClosestTownAndDensity(const World::Pos2& loc)
    {
        int32_t closestDistance = std::numeric_limits<uint16_t>::max();
        auto closestTown = TownId::null; // ebx
        for (const auto& town : towns())
        {
            const auto distance = Math::Vector::manhattanDistance(World::Pos2(town.x, town.y), loc);
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
        const int32_t realDistance = Math::Vector::distance(World::Pos2(town->x, town->y), loc);
        // Works out a proxiy for how likely there is to be buildings at the location
        // i.e. how dense the area is.
        const auto unk = std::clamp((realDistance - town->numBuildings * 4 + 512) / 128, 0, 4);
        const uint8_t density = std::min(4 - unk, 3); // edx
        return { std::make_pair(town->id(), density) };
    }

    void registerHooks()
    {
        registerHook(
            0x00497348,
            []([[maybe_unused]] registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                resetBuildingsInfluence();
                return 0;
            });

        registerHook(
            0x00497FFC,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backupRegs = regs;
                Town* town = X86Pointer<Town>(regs.esi);
                auto res = town->findRoadExtent();
                if (res.has_value())
                {
                    backupRegs.ax = res->roadStart.x;
                    backupRegs.cx = res->roadStart.y;
                    backupRegs.dx = res->roadStart.z;
                    static loco_global<uint16_t, 0x001135C5A> _trackAndDirection;
                    _trackAndDirection = res->tad | (res->isBridge ? 1 << 12 : 0);
                    backupRegs.ebp = res->tad;
                }
                else
                {
                    backupRegs.ax = -1;
                }
                regs = backupRegs;
                return 0;
            });

        registerHook(
            0x00498101,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backupRegs = regs;
                Town* town = X86Pointer<Town>(regs.esi);
                town->buildInitialRoad();
                regs = backupRegs;
                return 0;
            });
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
