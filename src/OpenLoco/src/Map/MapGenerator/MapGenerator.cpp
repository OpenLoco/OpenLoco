#include "MapGenerator.h"
#include "../SurfaceElement.h"
#include "../Tile.h"
#include "../TileLoop.hpp"
#include "../TileManager.h"
#include "../Tree.h"
#include "../TreeElement.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Town/CreateTown.h"
#include "GameState.h"
#include "LastGameOptionManager.h"
#include "Localisation/StringIds.h"
#include "Objects/BuildingObject.h"
#include "Objects/HillShapesObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "OriginalTerrainGenerator.h"
#include "Random.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "SimplexTerrainGenerator.h"
#include "Ui/ProgressBar.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>
#include <cstdint>
#include <random>
#include <vector>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::Ui;
using namespace OpenLoco::S5;
using namespace OpenLoco::World::MapGenerator;

namespace OpenLoco::World::MapGenerator
{
    static loco_global<uint8_t*, 0x00F00160> _heightMap;

    // 0x004624F0
    static void generateHeightMap(const S5::Options& options, HeightMap& heightMap)
    {
        if (options.generator == LandGeneratorType::Original)
        {
            OriginalTerrainGenerator generator;
            generator.generate(options, heightMap);
        }
        else
        {
            SimplexTerrainGenerator generator;
            generator.generate(options, heightMap, std::random_device{}());
        }
    }

    // 0x004625D0
    static void generateLand(HeightMap& heightMap)
    {
        for (auto pos : World::getDrawableTileRange())
        {
            const uint8_t q00 = heightMap[{ pos.x + 0, pos.y + 0 }];
            const uint8_t q01 = heightMap[{ pos.x + 0, pos.y + 1 }];
            const uint8_t q10 = heightMap[{ pos.x + 1, pos.y + 0 }];
            const uint8_t q11 = heightMap[{ pos.x + 1, pos.y + 1 }];

            const auto tile = TileManager::get(pos);
            auto* surfaceElement = tile.surface();
            if (surfaceElement == nullptr)
            {
                continue;
            }

            const uint8_t baseHeight = (q00 + q01 + q10 + q11) / 4;
            surfaceElement->setBaseZ(std::max(2, baseHeight * kSmallZStep));

            uint8_t currentSlope = SurfaceSlope::flat;

            // First, figure out basic corner style
            if (q00 > baseHeight)
                currentSlope |= SurfaceSlope::CornerUp::south;
            if (q01 > baseHeight)
                currentSlope |= SurfaceSlope::CornerUp::west;
            if (q10 > baseHeight)
                currentSlope |= SurfaceSlope::CornerUp::east;
            if (q11 > baseHeight)
                currentSlope |= SurfaceSlope::CornerUp::north;

            // Now, deduce if we should go for double height
            if ((currentSlope == SurfaceSlope::CornerDown::north && q00 - baseHeight >= 2) ||
                (currentSlope == SurfaceSlope::CornerDown::east  && q01 - baseHeight >= 2) ||
                (currentSlope == SurfaceSlope::CornerDown::west  && q10 - baseHeight >= 2) ||
                (currentSlope == SurfaceSlope::CornerDown::south && q11 - baseHeight >= 2))
            {
                currentSlope |= SurfaceSlope::doubleHeight;
            }

            surfaceElement->setSlope(currentSlope);

            auto clearZ = surfaceElement->baseZ();
            if (surfaceElement->slopeCorners())
            {
                clearZ += kSmallZStep;
            }
            if (surfaceElement->isSlopeDoubleHeight())
            {
                clearZ += kSmallZStep;
            }
            surfaceElement->setClearZ(clearZ);
        }
    }

    // 0x004C4BD7
    static void generateWater([[maybe_unused]] HeightMap& heightMap)
    {
        auto seaLevel = getGameState().seaLevel;

        for (auto pos : World::getDrawableTileRange())
        {
            auto tile = TileManager::get(pos);
            auto* surface = tile.surface();

            if (surface != nullptr && surface->baseZ() < (seaLevel << 2))
                surface->setWater(seaLevel);
        }
    }

    static std::optional<uint8_t> getEverywhereSurfaceStyle()
    {
        for (uint8_t landObjectIdx = 0; landObjectIdx < ObjectManager::getMaxObjects(ObjectType::land); ++landObjectIdx)
        {
            auto* landObj = ObjectManager::get<LandObject>(landObjectIdx);
            if (landObj == nullptr)
            {
                continue;
            }
            if (S5::getOptions().landDistributionPatterns[landObjectIdx] == LandDistributionPattern::everywhere)
            {
                return landObjectIdx;
            }
        }
        if (LastGameOptionManager::getLastLand() != LastGameOptionManager::kNoLastOption)
        {
            return LastGameOptionManager::getLastLand();
        }
        return std::nullopt;
    }

    // 0x00469FC8
    std::optional<uint8_t> getRandomTerrainVariation(const SurfaceElement& surface)
    {
        if (surface.water())
        {
            return std::nullopt;
        }
        if (surface.isIndustrial())
        {
            return std::nullopt;
        }

        auto* landObj = ObjectManager::get<LandObject>(surface.terrain());
        if (landObj == nullptr)
        {
            return std::nullopt;
        }

        if (landObj->numVariations == 0 || surface.slope())
        {
            return 0;
        }

        // TODO: split into two randNext calls
        uint16_t randVal = gPrng1().randNext();
        if (landObj->variationLikelihood <= (randVal >> 8))
        {
            return 0;
        }
        return ((randVal & 0xFF) * landObj->numVariations) >> 8;
    }

    // 0x0046A379
    static void generateTerrainFarFromWater(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A379, regs);
        _heightMap = nullptr;
    }

    // 0x0046A439
    static void generateTerrainNearWater(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A439, regs);
        _heightMap = nullptr;
    }

    // 0x0046A5B3
    static void generateTerrainOnMountains(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A5B3, regs);
        _heightMap = nullptr;
    }

    // 0x0046A4F9
    static void generateTerrainFarFromMountains(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A4F9, regs);
        _heightMap = nullptr;
    }

    // 0x0046A0D8
    static void generateTerrainInSmallRandomAreas(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A0D8, regs);
        _heightMap = nullptr;
    }

    // 0x0046A227
    static void generateTerrainInLargeRandomAreas(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A227, regs);
        _heightMap = nullptr;
    }

    // 0x0046A66D
    static void generateTerrainAroundCliffs(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A66D, regs);
        _heightMap = nullptr;
    }

    static void generateTerrainNull([[maybe_unused]] HeightMap& heightMap, [[maybe_unused]] uint8_t surfaceStyle) {}

    using GenerateTerrainFunc = void (*)(HeightMap&, uint8_t);
    static const GenerateTerrainFunc _generateFuncs[] = {
        generateTerrainNull,               // LandDistributionPattern::everywhere This is null as it is a special function performed separately
        generateTerrainNull,               // LandDistributionPattern::nowhere
        generateTerrainFarFromWater,       // LandDistributionPattern::farFromWater
        generateTerrainNearWater,          // LandDistributionPattern::nearWater
        generateTerrainOnMountains,        // LandDistributionPattern::onMountains
        generateTerrainFarFromMountains,   // LandDistributionPattern::farFromMountains
        generateTerrainInSmallRandomAreas, // LandDistributionPattern::inSmallRandomAreas
        generateTerrainInLargeRandomAreas, // LandDistributionPattern::inLargeRandomAreas
        generateTerrainAroundCliffs        // LandDistributionPattern::aroundCliffs
    };

    // 0x0046A021
    static void generateTerrain(HeightMap& heightMap)
    {
        const auto style = getEverywhereSurfaceStyle();
        if (!style.has_value())
        {
            return;
        }

        const TilePosRangeView tileLoop = getDrawableTileRange();
        for (const auto& tilePos : tileLoop)
        {
            auto* surface = World::TileManager::get(tilePos).surface();
            if (surface == nullptr)
            {
                continue;
            }
            surface->setTerrain(*style);
            surface->setVar6SLR5(0);

            const auto variation = getRandomTerrainVariation(*surface);
            if (variation.has_value())
            {
                surface->setVariation(*variation);
            }
        }

        for (const auto pattern : {
                 LandDistributionPattern::farFromWater,
                 LandDistributionPattern::nearWater,
                 LandDistributionPattern::onMountains,
                 LandDistributionPattern::farFromMountains,
                 LandDistributionPattern::inSmallRandomAreas,
                 LandDistributionPattern::inLargeRandomAreas,
                 LandDistributionPattern::aroundCliffs,
             })
        {
            for (uint8_t landObjectIdx = 0; landObjectIdx < ObjectManager::getMaxObjects(ObjectType::land); ++landObjectIdx)
            {
                const auto* landObj = ObjectManager::get<LandObject>(landObjectIdx);
                if (landObj == nullptr)
                {
                    continue;
                }
                const auto typePattern = S5::getOptions().landDistributionPatterns[landObjectIdx];
                if (typePattern != pattern)
                {
                    continue;
                }
                _generateFuncs[enumValue(pattern)](heightMap, landObjectIdx);
            }
        }
    }

    // 0x004BE0C7
    static void updateTreeSeasons()
    {
        auto currentSeason = getGameState().currentSeason;

        for (auto pos : World::getDrawableTileRange())
        {
            auto tile = TileManager::get(pos);
            for (auto el : tile)
            {
                auto* treeEl = el.as<TreeElement>();
                if (treeEl == nullptr)
                {
                    continue;
                }

                if (treeEl->season() < 4 && !treeEl->unk6_80())
                {
                    treeEl->setSeason(enumValue(currentSeason));
                    treeEl->setUnk7l(0x7);
                }
                break;
            }
        }
    }

    // 0x004BDA49
    static void generateTrees()
    {
        const auto& options = S5::getOptions();

        // Place forests
        for (auto i = 0; i < options.numberOfForests; ++i)
        {
            const auto randRadius = ((gPrng1().randNext(255) * std::max(options.maxForestRadius - options.minForestRadius, 0)) / 255 + options.minForestRadius) * kTileSize;
            const auto randLoc = World::TilePos2(gPrng1().randNext(kMapRows), gPrng1().randNext(kMapColumns));
            const auto randDensity = (gPrng1().randNext(15) * std::max(options.maxForestDensity - options.minForestDensity, 0)) / 15 + options.minForestDensity;
            placeTreeCluster(randLoc, randRadius, randDensity, std::nullopt);

            if (TileManager::numFreeElements() < 0x36000)
            {
                break;
            }
        }

        // Place a number of random trees
        for (auto i = 0; i < options.numberRandomTrees; ++i)
        {
            const auto randLoc = World::Pos2(gPrng1().randNext(kMapWidth), gPrng1().randNext(kMapHeight));
            placeRandomTree(randLoc, std::nullopt);
        }

        // Cull trees that are too high / low
        uint32_t randMask = gPrng1().randNext();
        uint32_t i = 0;
        std::vector<TileElement*> toBeRemoved;
        for (auto& loc : getWorldRange())
        {
            auto tile = TileManager::get(loc);
            for (auto& el : tile)
            {
                auto* elTree = el.as<TreeElement>();
                if (elTree == nullptr)
                {
                    continue;
                }

                if (elTree->baseHeight() / kMicroToSmallZStep <= options.minAltitudeForTrees)
                {
                    if (elTree->baseHeight() / kMicroToSmallZStep != options.minAltitudeForTrees
                        || (randMask & (1 << i)))
                    {
                        toBeRemoved.push_back(&el);
                        i++;
                        i %= 32;
                        break;
                    }
                }

                if (elTree->baseHeight() / kMicroToSmallZStep >= options.maxAltitudeForTrees)
                {
                    if (elTree->baseHeight() / kMicroToSmallZStep != options.maxAltitudeForTrees
                        || (randMask & (1 << i)))
                    {
                        toBeRemoved.push_back(&el);
                        i++;
                        i %= 32;
                        break;
                    }
                }
            }

            // Remove in reverse order to prevent pointer invalidation
            for (auto elIter = std::rbegin(toBeRemoved); elIter != std::rend(toBeRemoved); ++elIter)
            {
                TileManager::removeElement(**elIter);
            }
            toBeRemoved.clear();
        }

        updateTreeSeasons();
    }

    // 0x00496BBC
    static void generateTowns()
    {
        for (auto i = 0; i < S5::getOptions().numberOfTowns; i++)
        {
            // NB: vanilla was calling the game command directly; we're using the runner.
            GameCommands::TownPlacementArgs args{};
            args.pos = { -1, -1 };
            const auto maxTownSize = S5::getOptions().maxTownSize;

            args.size = maxTownSize > 1 ? getGameState().rng.randNext(1, maxTownSize) : 1;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }

    // 0x004597FD
    static void generateIndustries(uint32_t minProgress, uint32_t maxProgress)
    {
        registers regs;
        regs.eax = minProgress;
        regs.ebx = maxProgress;
        call(0x004597FD, regs);
    }

    // 0x0042E731
    static void generateMiscBuildingType0(const BuildingObject* buildingObj, const size_t id)
    {
        registers regs;
        regs.ebp = X86Pointer(buildingObj);
        regs.ebx = id;
        call(0x0042E731, regs);
    }

    // 0x0042E893
    static void generateMiscBuildingType1(const BuildingObject* buildingObj, const size_t id)
    {
        registers regs;
        regs.ebp = X86Pointer(buildingObj);
        regs.ebx = id;
        call(0x0042E893, regs);
    }

    // 0x0042EA29
    static void generateMiscBuildingType2(const BuildingObject* buildingObj, const size_t id)
    {
        registers regs;
        regs.ebp = X86Pointer(buildingObj);
        regs.ebx = id;
        call(0x0042EA29, regs);
    }

    // 0x0042EB94
    static void generateMiscBuildingType3(const BuildingObject* buildingObj, const size_t id)
    {
        registers regs;
        regs.ebp = X86Pointer(buildingObj);
        regs.ebx = id;
        call(0x0042EB94, regs);
    }

    // 0x0042E6F2
    static void generateMiscBuildings()
    {
        GameCommands::setUpdatingCompanyId(CompanyId::neutral);

        for (auto id = 0U; id < ObjectManager::getMaxObjects(ObjectType::building); id++)
        {
            auto* buildingObj = ObjectManager::get<BuildingObject>(id);
            if (buildingObj == nullptr)
            {
                continue;
            }

            if (!buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
            {
                continue;
            }

            if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
            {
                continue;
            }

            static std::array<std::function<void(const BuildingObject*, const size_t)>, 4> generatorFunctions = {
                generateMiscBuildingType0,
                generateMiscBuildingType1,
                generateMiscBuildingType2,
                generateMiscBuildingType3,
            };

            generatorFunctions[buildingObj->generatorFunction](buildingObj, id);
        }
    }

    static void updateProgress(uint8_t value)
    {
        Ui::processMessagesMini();
        Ui::ProgressBar::setProgress(value);
    }

    // 0x0043C90C
    void generate(const S5::Options& options)
    {
        Ui::processMessagesMini();

        WindowManager::close(WindowType::town);
        WindowManager::close(WindowType::industry);
        Ui::ProgressBar::begin(StringIds::generating_landscape);

        auto rotation = WindowManager::getCurrentRotation();
        Scenario::reset();
        WindowManager::setCurrentRotation(rotation);

        updateProgress(5);

        Scenario::initialiseDate(options.scenarioStartYear);
        Scenario::initialiseSnowLine();
        TileManager::initialise();
        updateProgress(10);

        {
            // Should be 384x384 (but generateLandscape goes out of bounds?)
            HeightMap heightMap(512, 512, 512);

            generateHeightMap(options, heightMap);
            updateProgress(17);

            generateLand(heightMap);
            updateProgress(17);

            generateWater(heightMap);
            updateProgress(25);

            generateTerrain(heightMap);
            updateProgress(35);
        }

        call(0x004611DF);
        updateProgress(40);

        generateTrees();
        updateProgress(45);

        generateTowns();
        updateProgress(225);

        generateIndustries(225, 245);
        updateProgress(245);

        generateMiscBuildings();
        updateProgress(250);

        call(0x004611DF);
        updateProgress(255);

        Scenario::sub_4969E0(0);
        Scenario::sub_4748D4();
        Ui::ProgressBar::end();
    }
}
