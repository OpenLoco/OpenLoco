#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/ClimateObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "SceneManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Math/Vector.hpp>
#include <numeric>

namespace OpenLoco::IndustryManager
{
    static auto& rawIndustries() { return getGameState().industries; }
    static auto getTotalIndustriesCap() { return getGameState().numberOfIndustries; }
    uint8_t getFlags() { return getGameState().industryFlags; }

    constexpr int32_t kCloseIndustryDistanceMax = 480;
    constexpr int32_t kIndustryWithinClusterDistance = 960;
    constexpr int32_t kNumIndustryInCluster = 3;
    constexpr int32_t kFindRandomNewIndustryAttempts = 250;
    // Above this is classed as high ground
    constexpr Map::SmallZ kIndustryHighGroundMin = 48;
    // Below this is classed as low ground
    constexpr Map::SmallZ kIndustryLowGroundMax = 56;
    // Below this is classed as flat ground
    constexpr int32_t kIndustryFlatGroundMountainMax = 32;
    constexpr uint32_t kIndustryTilesToBeInDesertMin = 100;
    constexpr uint32_t kIndustryTilesToBeNearDesertMax = 70;
    constexpr uint32_t kIndustryTilesToBeNearWaterMin = 10;
    constexpr uint32_t kIndustryTilesToBeAwayWaterMax = 0;
    constexpr int32_t kIndustryDistToBeNearTownMax = 576;
    constexpr int32_t kIndustryDistToBeAwayTownMin = 768;
    constexpr uint32_t kIndustryNumTreesToBeNearTreesMin = 25;
    constexpr uint32_t kIndustryNumTressToBeOpenSpaceMax = 3;

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
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
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
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.updateDaily();
            }
        }
    }

    // 0x0047EA42
    static size_t getMostCommonBuildingCargoType()
    {
        // First generate a count of all the different cargo based on what building could generate
        std::array<uint32_t, ObjectManager::getMaxObjects(ObjectType::cargo)> cargoCounts{};
        for (size_t buildObjId = 0; buildObjId < ObjectManager::getMaxObjects(ObjectType::building); ++buildObjId)
        {
            const auto* buildObj = ObjectManager::get<BuildingObject>(buildObjId);
            if (buildObj == nullptr)
            {
                continue;
            }
            if (!(buildObj->flags & BuildingObjectFlags::miscBuilding)
                && buildObj->producedQuantity[0] != 0)
            {
                cargoCounts[buildObj->producedCargoType[0]]++;
            }
        }
        // Then pick the most common
        auto maxEl = std::max_element(std::begin(cargoCounts), std::end(cargoCounts));
        if (*maxEl != 0)
        {
            return std::distance(std::begin(cargoCounts), maxEl);
        }

        // If none are common pick any valid cargo object
        for (size_t cargoObjId = 0; cargoObjId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoObjId)
        {
            const auto* cargoObj = ObjectManager::get<CargoObject>(cargoObjId);
            if (cargoObj == nullptr)
            {
                continue;
            }
            return cargoObjId;
        }

        // This should really be an error!
        return 0;
    }

    // 0x0045960E
    static bool canCargoTypeBeProducedInWorld(uint8_t cargoType)
    {
        if (cargoType == getMostCommonBuildingCargoType())
        {
            return true;
        }

        for (auto& industry : industries())
        {
            const auto* indObj = industry.getObject();
            auto res = std::find(std::begin(indObj->producedCargoType), std::end(indObj->producedCargoType), cargoType);
            if (res != std::end(indObj->producedCargoType))
            {
                return true;
            }
        }
        return false;
    }

    static bool canIndustryObjBeCreated(const IndustryObject& indObj)
    {
        if (getCurrentYear() < indObj.designedYear)
        {
            return false;
        }

        if (getCurrentYear() >= indObj.obsoleteYear)
        {
            return false;
        }

        // If industry is a self producer i.e. no requirements to produce
        if (indObj.requiredCargoType[0] == 0xFF)
        {
            return true;
        }

        if (indObj.flags & IndustryObjectFlags::requiresAllCargo)
        {
            // All required cargo must be producable in world
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj.requiredCargoType[i] == 0xFF)
                {
                    continue;
                }
                if (!canCargoTypeBeProducedInWorld(indObj.requiredCargoType[i]))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            // At least one required cargo must be producable in world
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj.requiredCargoType[i] == 0xFF)
                {
                    continue;
                }
                if (canCargoTypeBeProducedInWorld(indObj.requiredCargoType[i]))
                {
                    return true;
                }
            }
            return false;
        }
    }

    // 0x00459A05
    static bool isTooCloseToNearbyIndustries(const Map::Pos2& loc)
    {
        for (auto& industry : industries())
        {
            const auto dist = Math::Vector::manhattanDistance(loc, Map::Pos2{ industry.x, industry.y });
            if (dist < kCloseIndustryDistanceMax)
            {
                return true;
            }
        }
        return false;
    }

    // 0x00459A50
    static bool isOutwithCluster(const Map::Pos2& loc, const uint8_t indObjId)
    {
        auto numClusters = 0;
        for (auto& industry : industries())
        {
            if (industry.objectId != indObjId)
            {
                continue;
            }
            numClusters++;
            const auto dist = Math::Vector::manhattanDistance(loc, Map::Pos2{ industry.x, industry.y });
            if (dist < kIndustryWithinClusterDistance)
            {
                return false;
            }
        }
        if (numClusters < kNumIndustryInCluster)
        {
            return false;
        }
        return true;
    }

    // 0x004599B3
    static std::optional<Map::Pos2> findRandomNewIndustryLocation(const uint8_t indObjId)
    {
        auto* indObj = ObjectManager::get<IndustryObject>(indObjId);
        for (auto i = 0; i < kFindRandomNewIndustryAttempts; ++i)
        {
            // Replace the below with this after validating the function
            // Map::Pos2 randomPos{
            //     Map::TilePos2(gPrng().randNext(Map::kMapRows), gPrng().randNext(Map::kMapColumns))
            // };
            const auto randomNum = gPrng().randNext();

            Map::Pos2 randomPos{
                Map::TilePos2(
                    (((randomNum >> 16) * Map::kMapRows) >> 16),
                    (((randomNum & 0xFFFF) * Map::kMapColumns) >> 16))
            };

            if (isTooCloseToNearbyIndustries(randomPos))
            {
                continue;
            }

            if (indObj->flags & IndustryObjectFlags::builtInClusters)
            {
                if (isOutwithCluster(randomPos, indObjId))
                {
                    continue;
                }
            }

            if (indObj->flags & IndustryObjectFlags::builtOnHighGround)
            {
                auto tile = Map::TileManager::get(randomPos);
                auto* surface = tile.surface();
                if (surface == nullptr || surface->baseZ() < kIndustryHighGroundMin)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtOnLowGround)
            {
                auto tile = Map::TileManager::get(randomPos);
                auto* surface = tile.surface();
                if (surface == nullptr || surface->baseZ() > kIndustryLowGroundMax)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtOnSnow)
            {
                auto tile = Map::TileManager::get(randomPos);
                auto* surface = tile.surface();
                auto* climateObj = ObjectManager::get<ClimateObject>();
                if (surface == nullptr || surface->baseZ() < climateObj->summerSnowLine)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtBelowSnowLine)
            {
                auto tile = Map::TileManager::get(randomPos);
                auto* surface = tile.surface();
                auto* climateObj = ObjectManager::get<ClimateObject>();
                if (surface == nullptr || surface->baseZ() > climateObj->winterSnowLine)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtOnFlatGround)
            {
                if (Map::TileManager::mountainHeight(randomPos) > kIndustryFlatGroundMountainMax)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtInDesert)
            {
                if (Map::TileManager::countSurroundingDesertTiles(randomPos) < kIndustryTilesToBeInDesertMin)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtNearDesert)
            {
                if (Map::TileManager::countSurroundingDesertTiles(randomPos) >= kIndustryTilesToBeNearDesertMax)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtNearWater)
            {
                if (Map::TileManager::countSurroundingWaterTiles(randomPos) < kIndustryTilesToBeNearWaterMin)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtAwayFromWater)
            {
                if (Map::TileManager::countSurroundingWaterTiles(randomPos) > kIndustryTilesToBeAwayWaterMax)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtOnWater)
            {
                auto tile = Map::TileManager::get(randomPos);
                auto* surface = tile.surface();
                if (surface != nullptr && surface->water() == 0)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtNearTown)
            {
                auto res = TownManager::getClosestTownAndDensity(randomPos);
                if (!res.has_value())
                {
                    continue;
                }
                const auto& [townId, density] = *res;
                if (density == 0)
                {
                    const auto* town = TownManager::get(townId);
                    if (Math::Vector::manhattanDistance(randomPos, Map::Pos2{ town->x, town->y }) > kIndustryDistToBeNearTownMax)
                    {
                        continue;
                    }
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtAwayFromTown)
            {
                auto res = TownManager::getClosestTownAndDensity(randomPos);
                if (!res.has_value())
                {
                    continue;
                }
                const auto townId = res->first;
                const auto* town = TownManager::get(townId);
                if (Math::Vector::manhattanDistance(randomPos, Map::Pos2{ town->x, town->y }) < kIndustryDistToBeAwayTownMin)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtNearTrees)
            {
                if (Map::TileManager::countSurroundingTrees(randomPos) < kIndustryNumTreesToBeNearTreesMin)
                {
                    continue;
                }
            }
            if (indObj->flags & IndustryObjectFlags::builtRequiresOpenSpace)
            {
                if (Map::TileManager::countSurroundingTrees(randomPos) > kIndustryNumTressToBeOpenSpaceMax)
                {
                    continue;
                }
            }
            return randomPos;
        }
        return std::nullopt;
    }

    // 0x00459722 & 0x004598F0
    static int32_t capOfTypeOfIndustry(const uint8_t indObjId)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(indObjId);

        // totalOfTypeInScenario is in the range of 1->32 inclusive
        // getTotalIndustriesCap() is in the range of 0->2 (low, med, high)
        // This formula is ultimately staticPreferredTotalOfType = 1/4 * ((getTotalIndustriesCap() + 1) * indObj->totalOfTypeInScenario)
        // but we will do it in two steps to keep identical results.
        const auto intermediate1 = ((getTotalIndustriesCap() + 1) * indObj->totalOfTypeInScenario) / 3;
        const auto staticPreferredTotalOfType = intermediate1 - intermediate1 / 4;
        // The preferred total can vary by up to a half of the static preffered total.
        const auto randomPreferredTotalOfType = (staticPreferredTotalOfType / 2) * gPrng().randNext(0xFF) / 256;
        return staticPreferredTotalOfType + randomPreferredTotalOfType;
    }

    // 0x00459722
    static bool hasReachedCapOfTypeOfIndustry(const uint8_t indObjId)
    {
        const auto preferredTotalOfType = capOfTypeOfIndustry(indObjId);

        const auto totalOfThisType = std::count_if(std::begin(industries()), std::end(industries()), [indObjId](const auto& industry) {
            return (industry.objectId == indObjId);
        });

        return totalOfThisType >= preferredTotalOfType;
    }

    // 0x0045979C & 0x00459949
    static void createNewIndustry(const uint8_t indObjId, const bool buildImmediately, const int32_t numAttempts)
    {
        // Try find valid coordinates for this industry
        for (auto attempt = 0; attempt < numAttempts; ++attempt)
        {
            auto randomIndustryLoc = findRandomNewIndustryLocation(indObjId);
            if (randomIndustryLoc.has_value())
            {
                GameCommands::IndustryPlacementArgs args;
                args.type = indObjId;
                args.buildImmediately = buildImmediately;
                args.pos = *randomIndustryLoc;
                // To match vanilla we will do this.
                // TODO: Once match confirmed replace with two randNext() calls
                const auto temp = gPrng().srand_0();
                gPrng().randNext();
                args.srand0 = gPrng().srand_0() - temp;
                args.srand1 = gPrng().srand_1();

                auto res = GameCommands::doCommand(args, GameCommands::Flags::apply);
                if (res != GameCommands::FAILURE)
                {
                    break;
                }
            }
        }
    }

    // 0x00459659
    static void tryCreateNewIndustriesMonthly()
    {
        if (getFlags() & Flags::disallowIndustriesStartUp)
        {
            return;
        }

        for (uint8_t indObjId = 0; static_cast<size_t>(indObjId) < ObjectManager::getMaxObjects(ObjectType::industry); ++indObjId)
        {
            auto* indObj = ObjectManager::get<IndustryObject>(indObjId);
            if (indObj == nullptr)
            {
                continue;
            }

            if (!canIndustryObjBeCreated(*indObj))
            {
                continue;
            }

            if (hasReachedCapOfTypeOfIndustry(indObjId))
            {
                continue;
            }
            createNewIndustry(indObjId, false, 25);
        }
    }

    // 0x0045383B
    void updateMonthly()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
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
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
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
            if (manhattanDistance / Map::kTileSize < 11)
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
