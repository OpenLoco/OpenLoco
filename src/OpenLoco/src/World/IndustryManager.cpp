#include "IndustryManager.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Industries/CreateIndustry.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/ClimateObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Random.h"
#include "SceneManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Math/Vector.hpp>
#include <numeric>

namespace OpenLoco::IndustryManager
{
    static auto& rawIndustries() { return getGameState().industries; }
    static auto getTotalIndustriesFactor() { return getGameState().numberOfIndustries; }
    Flags getFlags() { return getGameState().industryFlags; }

    constexpr int32_t kCloseIndustryDistanceMax = 480;
    constexpr int32_t kIndustryWithinClusterDistance = 960;
    constexpr int32_t kNumIndustryInCluster = 3;
    constexpr int32_t kFindRandomNewIndustryAttempts = 250;
    // Above this is classed as high ground
    constexpr World::SmallZ kIndustryHighGroundMin = 48;
    // Below this is classed as low ground
    constexpr World::SmallZ kIndustryLowGroundMax = 56;
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

    void setFlags(const Flags flags)
    {
        getGameState().industryFlags = flags;
    }

    bool hasFlags(const Flags flags)
    {
        return (getGameState().industryFlags & flags) != Flags::none;
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
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !SceneManager::isEditorMode())
        {
            GameCommands::setUpdatingCompanyId(CompanyId::neutral);
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
            GameCommands::setUpdatingCompanyId(CompanyId::neutral);
            for (auto& industry : industries())
            {
                industry.updateDaily();
            }
        }
    }

    // 0x0047EA42
    uint8_t getMostCommonBuildingCargoType()
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
            if (!buildObj->hasFlags(BuildingObjectFlags::miscBuilding)
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
        for (uint8_t cargoObjId = 0; cargoObjId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoObjId)
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

        if (indObj.hasFlags(IndustryObjectFlags::requiresAllCargo))
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
    static bool isTooCloseToNearbyIndustries(const World::Pos2& loc)
    {
        for (auto& industry : industries())
        {
            const auto dist = Math::Vector::manhattanDistance2D(loc, World::Pos2{ industry.x, industry.y });
            if (dist < kCloseIndustryDistanceMax)
            {
                return true;
            }
        }
        return false;
    }

    // 0x00459A50
    static bool isOutwithCluster(const World::Pos2& loc, const uint8_t indObjId)
    {
        auto numClusters = 0;
        for (auto& industry : industries())
        {
            if (industry.objectId != indObjId)
            {
                continue;
            }
            numClusters++;
            const auto dist = Math::Vector::manhattanDistance2D(loc, World::Pos2{ industry.x, industry.y });
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
    static std::optional<World::Pos2> findRandomNewIndustryLocation(const uint8_t indObjId)
    {
        auto* indObj = ObjectManager::get<IndustryObject>(indObjId);
        for (auto i = 0; i < kFindRandomNewIndustryAttempts; ++i)
        {
            // Replace the below with this after validating the function
            // Map::Pos2 randomPos{
            //     Map::TilePos2(gPrng1().randNext(Map::World::TileManager::getMapRows()), gPrng1().randNext(Map::World::TileManager::getMapColumns()))
            // };
            const auto randomNum = gPrng1().randNext();

            const auto randomPos = World::toWorldSpace(World::TilePos2(
                (((randomNum >> 16) * World::TileManager::getMapRows()) >> 16),
                (((randomNum & 0xFFFF) * World::TileManager::getMapColumns()) >> 16)));

            if (isTooCloseToNearbyIndustries(randomPos))
            {
                continue;
            }

            if (indObj->hasFlags(IndustryObjectFlags::builtInClusters))
            {
                if (isOutwithCluster(randomPos, indObjId))
                {
                    continue;
                }
            }

            if (indObj->hasFlags(IndustryObjectFlags::builtOnHighGround))
            {
                auto tile = World::TileManager::get(randomPos);
                auto* surface = tile.surface();
                if (surface == nullptr || surface->baseZ() < kIndustryHighGroundMin)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtOnLowGround))
            {
                auto tile = World::TileManager::get(randomPos);
                auto* surface = tile.surface();
                if (surface == nullptr || surface->baseZ() > kIndustryLowGroundMax)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtOnSnow))
            {
                auto tile = World::TileManager::get(randomPos);
                auto* surface = tile.surface();
                auto* climateObj = ObjectManager::get<ClimateObject>();
                if (surface == nullptr || surface->baseZ() < climateObj->summerSnowLine)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtBelowSnowLine))
            {
                auto tile = World::TileManager::get(randomPos);
                auto* surface = tile.surface();
                auto* climateObj = ObjectManager::get<ClimateObject>();
                if (surface == nullptr || surface->baseZ() > climateObj->winterSnowLine)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtOnFlatGround))
            {
                if (World::TileManager::mountainHeight(randomPos) > kIndustryFlatGroundMountainMax)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtInDesert))
            {
                if (World::TileManager::countSurroundingDesertTiles(randomPos) < kIndustryTilesToBeInDesertMin)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtNearDesert))
            {
                if (World::TileManager::countSurroundingDesertTiles(randomPos) >= kIndustryTilesToBeNearDesertMax)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtNearWater))
            {
                if (World::TileManager::countSurroundingWaterTiles(randomPos) < kIndustryTilesToBeNearWaterMin)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtAwayFromWater))
            {
                if (World::TileManager::countSurroundingWaterTiles(randomPos) > kIndustryTilesToBeAwayWaterMax)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtOnWater))
            {
                auto tile = World::TileManager::get(randomPos);
                auto* surface = tile.surface();
                if (surface != nullptr && surface->water() == 0)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtNearTown))
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
                    if (Math::Vector::manhattanDistance2D(randomPos, World::Pos2{ town->x, town->y }) > kIndustryDistToBeNearTownMax)
                    {
                        continue;
                    }
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtAwayFromTown))
            {
                auto res = TownManager::getClosestTownAndDensity(randomPos);
                if (!res.has_value())
                {
                    continue;
                }
                const auto townId = res->first;
                const auto* town = TownManager::get(townId);
                if (Math::Vector::manhattanDistance2D(randomPos, World::Pos2{ town->x, town->y }) < kIndustryDistToBeAwayTownMin)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtNearTrees))
            {
                if (World::TileManager::countSurroundingTrees(randomPos) < kIndustryNumTreesToBeNearTreesMin)
                {
                    continue;
                }
            }
            if (indObj->hasFlags(IndustryObjectFlags::builtRequiresOpenSpace))
            {
                if (World::TileManager::countSurroundingTrees(randomPos) > kIndustryNumTressToBeOpenSpaceMax)
                {
                    continue;
                }
            }
            return randomPos;
        }
        return std::nullopt;
    }

    // 0x00459722 & 0x004598F0
    int32_t capOfTypeOfIndustry(const uint8_t indObjId, const uint8_t numIndustriesFactor)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(indObjId);

        // totalOfTypeInScenario is in the range of 1->32 inclusive
        // numIndustriesFactor is in the range of 0->2 (low, med, high)
        // This formula is ultimately staticPreferredTotalOfType = 1/4 * ((numIndustriesFactor + 1) * indObj->totalOfTypeInScenario)
        // but we will do it in two steps to keep identical results.
        const auto intermediate1 = ((numIndustriesFactor + 1) * indObj->totalOfTypeInScenario) / 3;
        const auto staticPreferredTotalOfType = intermediate1 - intermediate1 / 4;

        // The preferred total can vary by up to a half of the static preferred total.
        const auto randomPreferredTotalOfType = (staticPreferredTotalOfType / 2) * (gPrng1().randNext(0xFF) / 256);
        return staticPreferredTotalOfType + randomPreferredTotalOfType;
    }

    // 0x00459722
    static bool hasReachedCapOfTypeOfIndustry(const uint8_t indObjId)
    {
        const uint8_t numIndustriesFactor = getTotalIndustriesFactor();
        const auto preferredTotalOfType = capOfTypeOfIndustry(indObjId, numIndustriesFactor);

        const auto totalOfThisType = std::count_if(std::begin(industries()), std::end(industries()), [indObjId](const auto& industry) {
            return (industry.objectId == indObjId);
        });

        return totalOfThisType >= preferredTotalOfType;
    }

    // 0x0045979C & 0x00459949
    void createNewIndustry(const uint8_t indObjId, const bool buildImmediately, const int32_t numAttempts)
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
                const auto temp = gPrng1().srand_0();
                gPrng1().randNext();
                args.srand0 = gPrng1().srand_0() - temp;
                args.srand1 = gPrng1().srand_1();

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
        if ((getFlags() & Flags::disallowIndustriesStartUp) != Flags::none)
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
            GameCommands::setUpdatingCompanyId(CompanyId::neutral);
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
        {
            return;
        }

        for (auto& industry : industries())
        {
            industry.createMapAnimations();
        }
    }

    // 0x048FE92
    bool industryNearPosition(const World::Pos2& position, IndustryObjectFlags flags)
    {
        for (auto& industry : industries())
        {
            const auto* industryObj = industry.getObject();
            if (!industryObj->hasFlags(flags))
            {
                continue;
            }

            auto manhattanDistance = Math::Vector::manhattanDistance2D(World::Pos2{ industry.x, industry.y }, position);
            if (manhattanDistance / World::kTileSize < 11)
            {
                return true;
            }
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

    // 0x00454DBE
    IndustryId allocateNewIndustry(const uint8_t type, const World::Pos2& pos, const Core::Prng& prng, const TownId nearbyTown)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(type);

        for (auto i = 0U; i < Limits::kMaxIndustries; ++i)
        {
            const auto id = static_cast<IndustryId>(i);
            auto* industry = IndustryManager::get(id);
            if (!industry->empty())
            {
                continue;
            }

            industry->prng = prng;
            industry->flags = IndustryFlags::none;
            industry->objectId = type;
            industry->x = pos.x;
            industry->y = pos.y;
            industry->numTiles = 0;
            industry->under_construction = 0;
            industry->tileLoop = World::TileLoop{};
            industry->numFarmTiles = 0;
            industry->numIdleFarmTiles = 0;
            industry->productionRate = 25;
            industry->foundingYear = getCurrentYear();
            industry->stationsInRange = {};
            for (auto& stats : industry->producedCargoStatsStation)
            {
                std::fill(std::begin(stats), std::end(stats), StationId::null);
            }
            std::fill(std::begin(industry->dailyProduction), std::end(industry->dailyProduction), 0);
            std::fill(std::begin(industry->outputBuffer), std::end(industry->outputBuffer), 0);
            std::fill(std::begin(industry->producedCargoQuantityMonthlyTotal), std::end(industry->producedCargoQuantityMonthlyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityPreviousMonth), std::end(industry->producedCargoQuantityPreviousMonth), 0);
            std::fill(std::begin(industry->receivedCargoQuantityMonthlyTotal), std::end(industry->receivedCargoQuantityMonthlyTotal), 0);
            std::fill(std::begin(industry->receivedCargoQuantityPreviousMonth), std::end(industry->receivedCargoQuantityPreviousMonth), 0);
            std::fill(std::begin(industry->receivedCargoQuantityDailyTotal), std::end(industry->receivedCargoQuantityDailyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityDeliveredMonthlyTotal), std::end(industry->producedCargoQuantityDeliveredMonthlyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityDeliveredPreviousMonth), std::end(industry->producedCargoQuantityDeliveredPreviousMonth), 0);
            std::fill(std::begin(industry->producedCargoPercentTransportedPreviousMonth), std::end(industry->producedCargoPercentTransportedPreviousMonth), 0);
            std::fill(std::begin(industry->producedCargoMonthlyHistorySize), std::end(industry->producedCargoMonthlyHistorySize), 1);
            // Note: vanilla just set to 0 first entry change this when allowing divergence
            // std::fill(std::begin(industry->producedCargoMonthlyHistory1), std::end(industry->producedCargoMonthlyHistory1), 0);
            // std::fill(std::begin(industry->producedCargoMonthlyHistory2), std::end(industry->producedCargoMonthlyHistory2), 0);
            industry->producedCargoMonthlyHistory1[0] = 0;
            industry->producedCargoMonthlyHistory2[0] = 0;
            std::fill(std::begin(industry->history_min_production), std::end(industry->history_min_production), 0);

            industry->town = nearbyTown;
            industry->name = indObj->var_02;

            for (auto& innerInd : IndustryManager::industries())
            {
                if (innerInd.name != industry->name)
                {
                    continue;
                }
                if (&innerInd == industry)
                {
                    continue;
                }
                if (innerInd.town != industry->town)
                {
                    continue;
                }

                for (auto unique = 1; unique < 0xFFF; ++unique)
                {
                    FormatArguments args{};
                    args.push<uint16_t>(unique);
                    char buffer[512]{};
                    StringManager::formatString(buffer, indObj->var_02 + 1, args);
                    const auto newName = StringManager::userStringAllocate(buffer, true);
                    if (newName == StringIds::empty)
                    {
                        continue;
                    }
                    industry->name = newName;
                    break;
                }
            }
            return id;
        }
        return IndustryId::null;
    }
}

OpenLoco::IndustryId OpenLoco::Industry::id() const
{
    auto* first = &IndustryManager::rawIndustries()[0];
    return IndustryId(this - first);
}
