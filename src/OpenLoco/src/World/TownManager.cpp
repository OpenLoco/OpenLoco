#include "TownManager.h"
#include "CompanyManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "Map/BuildingElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TownNamesObject.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::TownManager
{
    static loco_global<Town*, 0x01135C38> _dword_1135C38;

    // 0x0049B45F
    static uint32_t calcCargoInfluenceFlags(Town* town)
    {
        registers regs;
        regs.esi = X86Pointer(town);
        call(0x0049B45F, regs);
        return regs.eax;
    }

    enum class LocationFlags : uint8_t
    {
        none = 0,
        adjacentToLargeWaterBody = 1 << 0,
        notMountaineous = 1 << 1,
        adjacentToSmallWaterBody = 1 << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(LocationFlags);

    // 0x00497D70
    static LocationFlags copyTownNameToBuffer(const TownNamesObject* namesObj, uint32_t categoryOffset, uint16_t index, char* buffer)
    {
        // Offset into the string table for the requested category, located just after the names object header.
        auto* offsetPtr = reinterpret_cast<const std::byte*>(namesObj) + categoryOffset;
        auto srcOffset = *reinterpret_cast<const int16_t*>(offsetPtr + index * 2);

        auto* srcPtr = reinterpret_cast<const char*>(offsetPtr + srcOffset);
        strcpy(buffer, srcPtr);

        // Location flags are stored at the end of the string
        LocationFlags flags = *reinterpret_cast<const LocationFlags*>(srcPtr + strlen(srcPtr) + 1);
        return flags;
    }

    // 0x00497A6A
    static LocationFlags townNameFromNamesObject(uint32_t rand, const char* buffer)
    {
        auto* namesObj = ObjectManager::get<TownNamesObject>();
        LocationFlags locationFlags = LocationFlags::none;

        // Town names are concatenated from six morpheme categories:
        // {CAT1}{CAT2}{CAT3}{CAT4}{CAT5}{CAT6}
        // Seperators are defined in each of the category strings; either '-', ' ', or nothing.
        // Categories can be completely empty, or can be skipped based on randomness.
        // e.g. "Fort " + "Apple " + "Green" for "Fort Apple Green"

        for (auto& category : namesObj->categories)
        {
            if (category.count == 0)
            {
                continue;
            }

            uint16_t ax = rand;
            uint16_t dx = category.count + category.bias;
            int16_t index = ((ax * dx) >> 16) - category.bias;

            if (index > 0)
            {
                char* strEnd = const_cast<char*>(buffer + strlen(buffer));
                locationFlags |= copyTownNameToBuffer(namesObj, category.offset, index, strEnd);
            }

            for (auto shifts = category.count + category.bias; shifts > 0; shifts >>= 1)
            {
                rand = std::rotr(rand, 1);
            }
        }

        return locationFlags;
    }

    // 0x004978B7
    static bool generateTownName(Town* town)
    {
        for (auto attemptsLeft = 400U; attemptsLeft > 0; attemptsLeft--)
        {
            char buffer[256]{};
            auto rand = town->prng.randNext();
            auto locationFlags = townNameFromNamesObject(rand, buffer);

            if (strlen(buffer) == 0)
            {
                continue;
            }

            if (strlen(buffer) > StringManager::kUserStringSize)
            {
                continue;
            }

            if (Gfx::TextRenderer::getStringWidth(Gfx::Font::medium_bold, buffer) > 200)
            {
                continue;
            }

            // clang-format off
            auto numSurroundingWaterTilesAboveThreshold = [](Pos2 pos, uint8_t threshold) {
                return TileManager::countSurroundingWaterTiles(pos + Pos2(6 * kTileSize, 0)) > threshold ||
                    TileManager::countSurroundingWaterTiles(pos + Pos2(0, 6 * kTileSize)) > threshold ||
                    TileManager::countSurroundingWaterTiles(pos + Pos2(0 - 6 * kTileSize, 0)) > threshold ||
                    TileManager::countSurroundingWaterTiles(pos + Pos2(0, 0 - 6 * kTileSize)) > threshold;
            };
            // clang-format on

            if ((locationFlags & LocationFlags::adjacentToLargeWaterBody) != LocationFlags::none)
            {
                // Check that the town is adjacent to a large amount of water tiles on at least one side.
                auto pos = Pos2(town->x, town->y);
                if (!(numSurroundingWaterTilesAboveThreshold(pos, 65)))
                {
                    continue;
                }
            }

            if ((locationFlags & LocationFlags::notMountaineous) != LocationFlags::none)
            {
                auto pos = Pos2(town->x + kTileSize / 2, town->y + kTileSize / 2);
                auto height = TileManager::getHeight(pos);
                if (height.landHeight < 192)
                {
                    continue;
                }
            }

            if ((locationFlags & LocationFlags::adjacentToSmallWaterBody) != LocationFlags::none)
            {
                // Check that the town is adjacent to a low amount of water tiles on at least one side.
                auto pos = Pos2(town->x, town->y);
                if (!(numSurroundingWaterTilesAboveThreshold(pos, 15)))
                {
                    continue;
                }
            }

            bool nameInUse = false;
            for (auto& candidateTown : towns())
            {
                // Ensure the town name doesn't exist yet
                char candidateTownName[256]{};
                StringManager::formatString(candidateTownName, candidateTown.name);

                if (strcmp(buffer, candidateTownName) == 0)
                {
                    nameInUse = true;
                    break;
                }
            }

            if (nameInUse)
            {
                continue;
            }

            StringId newNameId = StringManager::userStringAllocate(buffer, true);
            if (newNameId == StringIds::empty)
            {
                continue;
            }

            town->name = newNameId;
            town->updateLabel();
            return true;
        }

        return false;
    }

    static auto& rawTowns() { return getGameState().towns; }

    // 0x00496FE7
    Town* initialiseTown(World::Pos2 pos)
    {
        Town* town = nullptr;
        for (auto& candidateTown : rawTowns())
        {
            if (candidateTown.empty())
            {
                town = &candidateTown;
                break;
            }
        }

        // No space for a new town?
        if (town == nullptr)
        {
            return nullptr;
        }

        // Initialise the new town
        town->x = pos.x;
        town->y = pos.y;
        town->flags = TownFlags::none;
        town->population = 0;
        town->populationCapacity = 0;
        town->numBuildings = 0;
        town->size = TownSize::hamlet;
        town->historySize = 1;
        town->history[0] = 0;
        town->historyMinPopulation = 0;

        std::fill_n(&town->var_150[0], std::size(town->var_150), 0);

        town->var_19C[0][0] = 0;
        town->var_19C[0][1] = 0;
        town->var_19C[1][0] = 0;
        town->var_19C[1][1] = 0;
        town->numStations = 0;
        town->numberOfAirports = 0;
        town->var_1A8 = 0;

        town->prng = getGameState().rng;

        std::fill_n(&town->companyRatings[0], std::size(town->companyRatings), 500);

        town->companiesWithRating = 0;

        std::fill_n(&town->monthlyCargoDelivered[0], std::size(town->monthlyCargoDelivered), 0);

        town->cargoInfluenceFlags = calcCargoInfluenceFlags(town);
        town->buildSpeed = 1;

        // Figure out a name for this town?
        if (!generateTownName(town))
        {
            town->name = StringIds::null;
            return nullptr;
        }

        // Figure out if we need to reset building influence
        for (auto& otherTown : towns())
        {
            if (otherTown.numBuildings == 0 && otherTown.population == 0 && otherTown.populationCapacity == 0)
            {
                continue;
            }

            resetBuildingsInfluence();
            break;
        }

        return town;
    }

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

        if (town == nullptr)
        {
            return nullptr;
        }

        town->populationCapacity += populationCapacity;

        if (population != 0)
        {
            town->population += population;
            Ui::WindowManager::invalidate(Ui::WindowType::townList);
            Ui::WindowManager::invalidate(Ui::WindowType::town, enumValue(town->id()));
        }
        if (rating != 0)
        {
            auto companyId = GameCommands::getUpdatingCompanyId();
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

        for (const auto& tilePos : World::getWorldRange())
        {
            auto tile = World::TileManager::get(tilePos);
            for (auto& element : tile)
            {
                auto* building = element.as<World::BuildingElement>();
                if (building == nullptr)
                {
                    continue;
                }

                if (building->isGhost())
                {
                    continue;
                }

                if (building->isMiscBuilding())
                {
                    continue;
                }

                if (building->sequenceIndex() != 0)
                {
                    continue;
                }

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
                auto* town = updateTownInfo(World::toWorldSpace(tilePos), population, producedQuantity, 0, 1);
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
                    GameCommands::setUpdatingCompanyId(CompanyId::neutral);
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
            const auto distance = Math::Vector::manhattanDistance2D(World::Pos2(town.x, town.y), loc);
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
        const int32_t realDistance = Math::Vector::distance2D(World::Pos2(town->x, town->y), loc);
        // Works out a proxy for how likely there is to be buildings at the location
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
                registers backup = regs;
                resetBuildingsInfluence();
                regs = backup;
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
