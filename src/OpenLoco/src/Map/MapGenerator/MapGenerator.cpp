#include "MapGenerator.h"
#include "../SurfaceElement.h"
#include "../Tile.h"
#include "../TileLoop.hpp"
#include "../TileManager.h"
#include "../Tree.h"
#include "../TreeElement.h"
#include "Date.h"
#include "GameCommands/Buildings/CreateBuilding.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Town/CreateTown.h"
#include "GameState.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Objects/BuildingObject.h"
#include "Objects/HillShapesObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "OriginalTerrainGenerator.h"
#include "PngTerrainGenerator.h"
#include "Random.h"
#include "Scenario.h"
#include "ScenarioOptions.h"
#include "SimplexTerrainGenerator.h"
#include "Ui/ProgressBar.h"
#include "Ui/WindowManager.h"
#include "World/TownManager.h"
#include <cassert>
#include <cstdint>
#include <random>
#include <vector>

using namespace OpenLoco::World;
using namespace OpenLoco::Ui;
using namespace OpenLoco::S5;
using namespace OpenLoco::World::MapGenerator;

namespace OpenLoco::World::MapGenerator
{
    static constexpr auto kCliffTerrainHeightDiff = 4;
    static constexpr auto kMountainTerrainHeight = 26;

    static fs::path _pngHeightmapPath{};

    static void updateProgress(uint8_t value)
    {
        Input::processMessagesMini();
        Ui::ProgressBar::setProgress(value);
    }

    // 0x004624F0
    static void generateHeightMap(const Scenario::Options& options, HeightMap& heightMap)
    {
        if (options.generator == Scenario::LandGeneratorType::Original)
        {
            OriginalTerrainGenerator generator;
            generator.generate(options, heightMap);
        }
        else if (options.generator == Scenario::LandGeneratorType::Simplex)
        {
            SimplexTerrainGenerator generator;
            generator.generate(options, heightMap, std::random_device{}());
        }
        else
        {
            PngTerrainGenerator generator;
            generator.generate(options, _pngHeightmapPath, heightMap);
        }
    }

    static void generateRivers(const Scenario::Options& options, HeightMap& heightMap)
    {
        for (auto i = 0; i < options.numRiverbeds; i++)
        {
            auto& gs = getGameState();
            const auto riverEastWest = gs.rng.randBool();
            const auto riverWidth = gs.rng.randNext(options.minRiverWidth, options.maxRiverWidth);
            const auto riverbedHeight = std::max<uint8_t>(gs.seaLevel > 0 ? gs.seaLevel - 1 : 1, options.minLandHeight);

            // We'll be varying the bank width as we meander
            auto riverbankWidth = options.riverbankWidth;
            auto totalRiverWidth = riverWidth + 2 * riverbankWidth;
            auto easternBankOffset = riverWidth + riverbankWidth;

            // Pivot: generate a random X position
            auto xStartPos = getGameState().rng.randNext(0.15 * heightMap.width, 0.85 * heightMap.width);
            for (auto yPos = 0; yPos < heightMap.height; yPos++)
            {
                for (auto xOffset = 0; xOffset < totalRiverWidth; xOffset++)
                {
                    auto pos = TilePos2(xStartPos + xOffset, yPos);
                    if (!riverEastWest)
                    {
                        pos = TilePos2(pos.y, pos.x);
                    }

                    if (!TileManager::validCoords(pos))
                    {
                        // We might meander back to a valid position later,
                        // so we're only breaking out of the inner loop.
                        break;
                    }

                    if (riverbankWidth > 0 && xOffset < riverbankWidth)
                    {
                        // Western riverbank (high to low)
                        auto bankPos = riverbankWidth - xOffset;
                        auto bankHeight = heightMap[pos] * bankPos / riverbankWidth;
                        heightMap[pos] = std::max<uint8_t>(riverbedHeight, bankHeight);
                    }
                    else if (riverbankWidth > 0 && xOffset > easternBankOffset)
                    {
                        // Eastern riverbank (low to high)
                        auto bankPos = xOffset - easternBankOffset;
                        auto bankHeight = heightMap[pos] * bankPos / riverbankWidth;
                        heightMap[pos] = std::max<uint8_t>(riverbedHeight, bankHeight);
                    }
                    else
                    {
                        // Simply carve out the river
                        heightMap[pos] = riverbedHeight;
                    }
                }

                // Let the river meander slightly
                const auto meanderRate = options.riverMeanderRate;
                if (meanderRate > 0 && yPos % 4 == 0)
                {
                    const auto halfMeanderRate = meanderRate / 2;

                    int8_t meanderOffset = getGameState().rng.randNext(0, meanderRate) - halfMeanderRate;
                    xStartPos += meanderOffset;

                    // Adjust bank width slightly as well
                    if (options.riverbankWidth > 0 && halfMeanderRate != 0)
                    {
                        riverbankWidth += meanderOffset / halfMeanderRate;
                        easternBankOffset += meanderOffset / halfMeanderRate;
                        totalRiverWidth += meanderOffset / halfMeanderRate * 2;
                    }
                }
            }
        }
    }

    // 0x004625D0
    static void generateLand(HeightMap& heightMap)
    {
        for (auto& pos : World::getDrawableTileRange())
        {
            const MicroZ q00 = heightMap[pos + TilePos2{ -1, -1 }];
            const MicroZ q01 = heightMap[pos + TilePos2{ 0, -1 }];
            const MicroZ q10 = heightMap[pos + TilePos2{ -1, 0 }];
            const MicroZ q11 = heightMap[pos + TilePos2{ 0, 0 }];

            const auto tile = TileManager::get(pos);
            auto* surfaceElement = tile.surface();
            if (surfaceElement == nullptr)
            {
                continue;
            }

            const MicroZ baseHeight = std::min({ q00, q01, q10, q11 });
            surfaceElement->setBaseZ(baseHeight * kMicroToSmallZStep);

            uint8_t currentSlope = SurfaceSlope::flat;

            // First, figure out basic corner style
            if (q00 > baseHeight)
            {
                currentSlope |= SurfaceSlope::CornerUp::south;
            }
            if (q01 > baseHeight)
            {
                currentSlope |= SurfaceSlope::CornerUp::east;
            }
            if (q10 > baseHeight)
            {
                currentSlope |= SurfaceSlope::CornerUp::west;
            }
            if (q11 > baseHeight)
            {
                currentSlope |= SurfaceSlope::CornerUp::north;
            }

            // Now, deduce if we should go for double height
            // clang-format off
            if ((currentSlope == SurfaceSlope::CornerDown::north && q00 - baseHeight >= 2) ||
                (currentSlope == SurfaceSlope::CornerDown::west  && q01 - baseHeight >= 2) ||
                (currentSlope == SurfaceSlope::CornerDown::east  && q10 - baseHeight >= 2) ||
                (currentSlope == SurfaceSlope::CornerDown::south && q11 - baseHeight >= 2))
            {
                currentSlope |= SurfaceSlope::doubleHeight;
            }
            // clang-format on

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

        for (auto& pos : World::getDrawableTileRange())
        {
            auto tile = TileManager::get(pos);
            auto* surface = tile.surface();

            if (surface != nullptr && surface->baseZ() < (seaLevel << 2))
            {
                surface->setWater(seaLevel);
            }
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
            if (Scenario::getOptions().landDistributionPatterns[landObjectIdx] == Scenario::LandDistributionPattern::everywhere)
            {
                return landObjectIdx;
            }
        }
        if (getGameState().lastLandOption != 0xFF)
        {
            return getGameState().lastLandOption;
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

    static void applySurfaceStyleToMarkedTiles(HeightMap& heightMap, uint8_t surfaceStyle, bool requireMark)
    {
        for (auto& pos : World::getDrawableTileRange())
        {
            const bool tileIsMarked = heightMap.isMarkerSet({ pos.x, pos.y });
            if (requireMark != tileIsMarked)
            {
                continue;
            }

            auto tile = TileManager::get(pos);
            auto* surface = tile.surface();
            if (surface == nullptr)
            {
                continue;
            }

            surface->setTerrain(surfaceStyle);
            auto res = getRandomTerrainVariation(*surface);
            if (res)
            {
                surface->setVariation(*res);
            }
        }
    }

    // 0x0046A379
    static void generateTerrainFarFromWater(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        heightMap.resetMarkerFlags();

        // Mark tiles near water
        auto seaLevel = getGameState().seaLevel;
        for (auto pos : getWorldRange())
        {
            auto height = heightMap.getHeight({ pos.x, pos.y });
            if (height > seaLevel)
            {
                continue;
            }

            for (auto lookaheadPos : getClampedRange(pos - TilePos2(25, 25), pos + TilePos2(25, 25)))
            {
                heightMap.setMarker({ lookaheadPos.x, lookaheadPos.y });
            }
        }

        // Apply surface style to tiles that have *not* been marked
        applySurfaceStyleToMarkedTiles(heightMap, surfaceStyle, false);
    }

    // 0x0046A439
    static void generateTerrainNearWater(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        heightMap.resetMarkerFlags();

        // Mark tiles near water
        auto seaLevel = getGameState().seaLevel;
        for (auto pos : getWorldRange())
        {
            auto height = heightMap.getHeight({ pos.x, pos.y });
            if (height < seaLevel)
            {
                continue;
            }

            for (auto lookaheadPos : getClampedRange(pos - TilePos2(25, 25), pos + TilePos2(25, 25)))
            {
                heightMap.setMarker({ lookaheadPos.x, lookaheadPos.y });
            }
        }

        // Apply surface style to tiles that have been marked
        applySurfaceStyleToMarkedTiles(heightMap, surfaceStyle, true);
    }

    // 0x0046A5B3
    static void generateTerrainOnMountains(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        heightMap.resetMarkerFlags();

        // Mark tiles above mountain level
        for (auto pos : getWorldRange())
        {
            // NB: this is an inclusive check to match vanilla
            auto height = heightMap.getHeight({ pos.x, pos.y });
            if (height <= kMountainTerrainHeight)
            {
                continue;
            }

            for (auto lookaheadPos : getClampedRange(pos - TilePos2(12, 12), pos + TilePos2(12, 12)))
            {
                heightMap.setMarker({ lookaheadPos.x, lookaheadPos.y });
            }
        }

        // Apply surface style to tiles that have been marked
        applySurfaceStyleToMarkedTiles(heightMap, surfaceStyle, true);
    }

    // 0x0046A4F9
    static void generateTerrainFarFromMountains(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        heightMap.resetMarkerFlags();

        // Mark tiles above mountain level
        for (auto pos : getWorldRange())
        {
            // NB: this is an exclusive check to match vanilla
            auto height = heightMap.getHeight({ pos.x, pos.y });
            if (height < kMountainTerrainHeight)
            {
                continue;
            }

            for (auto lookaheadPos : getClampedRange(pos - TilePos2(25, 25), pos + TilePos2(25, 25)))
            {
                heightMap.setMarker({ lookaheadPos.x, lookaheadPos.y });
            }
        }

        // Apply surface style to tiles that have *not* been marked
        applySurfaceStyleToMarkedTiles(heightMap, surfaceStyle, false);
    }

    // 0x0046A0D8, 0x0046A227
    static void generateTerrainInRandomAreas(uint8_t surfaceStyle, uint16_t minTiles, uint16_t maxTiles)
    {
        auto numberOfAreas = getGameState().rng.randNext(80, 143);
        for (auto i = 0; i < numberOfAreas; i++)
        {
            // TODO: could probably simplify / replace with two randNext(lo, hi) calls
            auto randPos = getGameState().rng.randNext();
            auto xPos = ((randPos & 0xFFFF) * TileManager::getMapColumns()) >> 16;
            auto yPos = ((randPos >> 16) * TileManager::getMapRows()) >> 16;

            auto pos = World::toWorldSpace(TilePos2(xPos, yPos));
            auto numberOfTiles = getGameState().rng.randNext(minTiles, maxTiles - 1);
            for (auto j = 0; j < numberOfTiles; j++)
            {
                if (TileManager::validCoords(pos))
                {
                    auto tile = TileManager::get(pos);
                    auto surface = tile.surface();
                    if (surface != nullptr)
                    {
                        auto height = TileManager::getSurfaceCornerHeight(*surface) / kMicroToSmallZStep;
                        if (height > getGameState().seaLevel)
                        {
                            surface->setTerrain(surfaceStyle);
                            auto variation = getRandomTerrainVariation(*surface);
                            if (variation.has_value())
                            {
                                surface->setVariation(variation.value());
                            }
                        }
                    }
                }

                auto randRotation = getGameState().rng.randNext(3);
                auto offset = kRotationOffset[randRotation];
                pos += offset;
            }
        }
    }

    // 0x0046A0D8
    static void generateTerrainInSmallRandomAreas([[maybe_unused]] HeightMap& heightMap, uint8_t surfaceStyle)
    {
        generateTerrainInRandomAreas(surfaceStyle, 24, 56);
    }

    // 0x0046A227
    static void generateTerrainInLargeRandomAreas([[maybe_unused]] HeightMap& heightMap, uint8_t surfaceStyle)
    {
        generateTerrainInRandomAreas(surfaceStyle, 128, 388);
    }

    // 0x0046A66D
    static void generateTerrainAroundCliffs(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        heightMap.resetMarkerFlags();

        // Mark tiles with sudden height changes in the next row
        for (auto pos : getDrawableTileRange())
        {
            auto heightA = heightMap.getHeight({ pos + TilePos2{ 0, 1 } });
            auto heightB = heightMap.getHeight({ pos + TilePos2{ 0, 1 } });

            // Find no cliff between A and B?
            if (std::abs(heightB - heightA) < kCliffTerrainHeightDiff)
            {
                auto heightC = heightMap.getHeight({ pos + TilePos2{ 0, 1 } });
                auto heightD = heightMap.getHeight({ pos + TilePos2{ 0, 1 } });

                // Find no cliff between C and D?
                if (std::abs(heightD - heightC) < kCliffTerrainHeightDiff)
                {
                    continue;
                }
            }

            // Found a cliff around this point, so mark the points around it
            for (auto lookaheadPos : getClampedRange(pos - TilePos2(6, 6), pos + TilePos2(6, 6)))
            {
                heightMap.setMarker({ lookaheadPos.x, lookaheadPos.y });
            }
        }

        // Apply surface style to tiles that have been marked
        applySurfaceStyleToMarkedTiles(heightMap, surfaceStyle, true);
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
            surface->setGrowthStage(0);

            const auto variation = getRandomTerrainVariation(*surface);
            if (variation.has_value())
            {
                surface->setVariation(*variation);
            }
        }

        constexpr std::array landDistributionPatterns = {
            Scenario::LandDistributionPattern::farFromWater,
            Scenario::LandDistributionPattern::nearWater,
            Scenario::LandDistributionPattern::onMountains,
            Scenario::LandDistributionPattern::farFromMountains,
            Scenario::LandDistributionPattern::inSmallRandomAreas,
            Scenario::LandDistributionPattern::inLargeRandomAreas,
            Scenario::LandDistributionPattern::aroundCliffs,
        };

        for (auto i = 0U; i < landDistributionPatterns.size(); i++)
        {
            updateProgress(55 + 12 * i);

            for (uint8_t landObjectIdx = 0; landObjectIdx < ObjectManager::getMaxObjects(ObjectType::land); ++landObjectIdx)
            {
                const auto* landObj = ObjectManager::get<LandObject>(landObjectIdx);
                if (landObj == nullptr)
                {
                    continue;
                }

                const auto typePattern = Scenario::getOptions().landDistributionPatterns[landObjectIdx];
                const auto distPattern = landDistributionPatterns[i];
                if (typePattern != distPattern)
                {
                    continue;
                }

                _generateFuncs[enumValue(distPattern)](heightMap, landObjectIdx);
            }
        }
    }

    // 0x004611DF
    static void generateSurfaceVariation()
    {
        for (auto& pos : World::getDrawableTileRange())
        {
            auto tile = TileManager::get(pos);
            auto* surface = tile.surface();

            if (surface == nullptr)
            {
                continue;
            }

            if (!surface->isIndustrial())
            {
                auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
                if (landObj->hasFlags(LandObjectFlags::unk0))
                {
                    bool setVariation = false;
                    if (surface->water())
                    {
                        auto waterBaseZ = surface->water() * kMicroToSmallZStep;
                        if (surface->slope())
                        {
                            waterBaseZ -= 4;
                        }

                        if (waterBaseZ > surface->baseZ())
                        {
                            if (surface->terrain() != 0)
                            {
                                surface->setGrowthStage(0);
                                setVariation = true;
                            }
                        }
                    }

                    if (!setVariation)
                    {
                        surface->setGrowthStage(landObj->numGrowthStages - 1);
                    }
                }
            }

            auto snowLine = Scenario::getCurrentSnowLine() / kMicroToSmallZStep;
            MicroZ baseMicroZ = (surface->baseZ() / kMicroToSmallZStep) + 1;
            auto unk = std::clamp(baseMicroZ - snowLine, 0, 5);
            surface->setSnowCoverage(unk);
        }
    }

    // 0x004BE0C7
    static void updateTreeSeasons()
    {
        auto currentSeason = getGameState().currentSeason;

        for (auto& pos : World::getDrawableTileRange())
        {
            auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                auto* treeEl = el.as<TreeElement>();
                if (treeEl == nullptr)
                {
                    continue;
                }

                if (treeEl->season() < 4 && !treeEl->isDying())
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
        const auto& options = Scenario::getOptions();

        // Place forests
        for (auto i = 0; i < options.numberOfForests; ++i)
        {
            const auto randRadius = ((gPrng1().randNext(255) * std::max(options.maxForestRadius - options.minForestRadius, 0)) / 255 + options.minForestRadius) * kTileSize;
            const auto randLoc = World::TilePos2(gPrng1().randNext(TileManager::getMapRows()), gPrng1().randNext(TileManager::getMapColumns()));
            const auto randDensity = (gPrng1().randNext(15) * std::max(options.maxForestDensity - options.minForestDensity, 0)) / 15 + options.minForestDensity;
            placeTreeCluster(randLoc, randRadius, randDensity, std::nullopt);

            if (TileManager::numFreeElements() < 0x1B000)
            {
                break;
            }
        }

        // Place a number of random trees
        for (auto i = 0; i < options.numberRandomTrees; ++i)
        {
            const auto randLoc = World::Pos2(gPrng1().randNext(TileManager::getMapWidth()), gPrng1().randNext(TileManager::getMapHeight()));
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
        for (auto i = 0; i < Scenario::getOptions().numberOfTowns; i++)
        {
            // NB: vanilla was calling the game command directly; we're using the runner.
            GameCommands::TownPlacementArgs args{};
            args.pos = { -1, -1 };
            const auto maxTownSize = Scenario::getOptions().maxTownSize;

            args.size = maxTownSize > 1 ? getGameState().rng.randNext(1, maxTownSize) : 1;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }

    // 0x004595B7
    static bool isCargoProducedAnywhere(uint8_t requiredCargoType)
    {
        auto commonBuildingCargoType = IndustryManager::getMostCommonBuildingCargoType();
        if (commonBuildingCargoType == requiredCargoType)
        {
            return true;
        }

        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto* industryObj = ObjectManager::get<IndustryObject>(i);
            if (industryObj == nullptr)
            {
                continue;
            }

            if (getCurrentYear() < industryObj->designedYear || getCurrentYear() >= industryObj->obsoleteYear)
            {
                continue;
            }

            for (auto producedCargoType : industryObj->producedCargoType)
            {
                if (producedCargoType == requiredCargoType)
                {
                    return true;
                }
            }
        }

        return false;
    }

    // 0x004597FD
    static void generateIndustries(uint32_t minProgress, uint32_t maxProgress)
    {
        auto numIndustriesAvailable = 0;
        for (auto indObjId = 0U; indObjId < ObjectManager::getMaxObjects(ObjectType::industry); indObjId++)
        {
            if (ObjectManager::get<IndustryObject>(indObjId) != nullptr)
            {
                numIndustriesAvailable++;
            }
        }

        if (numIndustriesAvailable == 0)
        {
            return;
        }

        GameCommands::setUpdatingCompanyId(CompanyId::neutral);

        auto progressTicksPerIndustry = (maxProgress - minProgress) / numIndustriesAvailable;
        auto currentProgress = minProgress;

        for (auto indObjId = 0U; indObjId < ObjectManager::getMaxObjects(ObjectType::industry); indObjId++)
        {
            auto* industryObj = ObjectManager::get<IndustryObject>(indObjId);
            if (industryObj == nullptr)
            {
                continue;
            }

            currentProgress += progressTicksPerIndustry;
            updateProgress(currentProgress);

            // Check if industry is available at present
            if (getCurrentYear() < industryObj->designedYear || getCurrentYear() >= industryObj->obsoleteYear)
            {
                continue;
            }

            // Check if required cargo is available
            if (industryObj->requiredCargoType[0] != 0xFF)
            {
                auto numCargoSpecified = 0;
                auto numCargoAvailable = 0;
                for (auto cargoType : industryObj->requiredCargoType)
                {
                    if (cargoType == 0xFF)
                    {
                        continue;
                    }

                    numCargoSpecified++;
                    if (isCargoProducedAnywhere(cargoType))
                    {
                        numCargoAvailable++;
                    }
                }

                if (industryObj->hasFlags(IndustryObjectFlags::requiresAllCargo))
                {
                    if (numCargoSpecified != numCargoAvailable)
                    {
                        continue;
                    }
                }
                else if (numCargoAvailable == 0)
                {
                    continue;
                }
            }

            const uint8_t numIndustriesFactor = Scenario::getOptions().numberOfIndustries;
            const auto numIndustriesToCreate = IndustryManager::capOfTypeOfIndustry(indObjId, numIndustriesFactor);
            for (auto i = 0; i < numIndustriesToCreate; i++)
            {
                IndustryManager::createNewIndustry(indObjId, true, 50);
            }
        }
    }

    template<typename Func>
    static void generateMiscBuilding(const BuildingObject* buildingObj, const size_t id, Func&& predicate)
    {
        uint8_t randomComponent = getGameState().rng.randNext(0, buildingObj->averageNumberOnMap / 2);
        uint8_t staticComponent = buildingObj->averageNumberOnMap - (buildingObj->averageNumberOnMap / 4);

        uint8_t amountToBuild = randomComponent + staticComponent;
        if (amountToBuild == 0)
        {
            return;
        }

        for (auto i = 0U; i < amountToBuild; i++)
        {
            for (auto attemptsLeft = 200; attemptsLeft > 0; attemptsLeft--)
            {
                // NB: coordinate selection has been simplified compared to vanilla
                auto randomX = getGameState().rng.randNext(2, TileManager::getMapRows() - 2);
                auto randomY = getGameState().rng.randNext(2, TileManager::getMapColumns() - 2);

                auto tile = TileManager::get(TilePos2(randomX, randomY));
                if (!predicate(tile))
                {
                    continue;
                }

                auto* surface = tile.surface();
                if (surface == nullptr)
                {
                    continue;
                }

                auto baseHeight = TileManager::getSurfaceCornerHeight(*surface) * kSmallZStep;

                auto randomRotation = getGameState().rng.randNext(0, 3);
                auto randomVariation = getGameState().rng.randNext(0, buildingObj->numVariations - 1);

                GameCommands::BuildingPlacementArgs args{};
                args.pos = Pos3(randomX * kTileSize, randomY * kTileSize, baseHeight);
                args.rotation = randomRotation;
                args.type = static_cast<uint8_t>(id);
                args.variation = randomVariation;
                args.colour = Colour::black;
                args.buildImmediately = true;

                if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
                {
                    break;
                }
            }
        }
    }

    // 0x0042E731
    // Example: 'Transmitter' building object
    static void generateMiscBuildingType0(const BuildingObject* buildingObj, const size_t id)
    {
        generateMiscBuilding(buildingObj, id, [](const Tile& tile) {
            // This kind of object (e.g. a transmitter) only occurs in mountains
            auto* surface = tile.surface();
            return surface->baseZ() >= 100;
        });
    }

    static World::Pos2 placeBuildingsAlongLine(World::Pos2 origin, uint8_t numTilesBetween, const size_t buildingId, int16_t remainingTilesDistance, uint8_t rotation)
    {
        // We'll be placing buildings along the line and return the final location.

        World::Pos2 targetPos = origin;
        // 0x0042E945
        for (remainingTilesDistance -= numTilesBetween; remainingTilesDistance > 0; remainingTilesDistance -= numTilesBetween)
        {
            // 0x0042E956
            auto offset = kRotationOffset[rotation] * numTilesBetween;
            targetPos += offset;
            if (!TileManager::drawableCoords(targetPos))
            {
                break;
            }

            // 0x0042E98A
            auto tile = TileManager::get(targetPos);
            auto* surface = tile.surface();
            if (surface == nullptr)
            {
                break;
            }

            auto height = TileManager::getSurfaceCornerHeight(*surface);

            GameCommands::BuildingPlacementArgs buildArgs{};
            buildArgs.pos = World::Pos3(targetPos, height * World::kSmallZStep);
            buildArgs.rotation = rotation;
            buildArgs.type = static_cast<uint8_t>(buildingId);
            buildArgs.variation = 0;
            buildArgs.colour = Colour::black;
            buildArgs.buildImmediately = true;

            // NB: return is ignored (it doesn't matter if some pylons missing)
            GameCommands::doCommand(buildArgs, GameCommands::Flags::apply);
        }

        return targetPos;
    }

    // 0x0042E893
    // Example: 'Electricity Pylon' building object
    // Only used by 'Coal-Fired Power Station' in vanilla
    static void generateMiscBuildingType1(const BuildingObject* buildingObj, const size_t id)
    {
        for (auto& industry : IndustryManager::industries())
        {
            auto* industryObj = ObjectManager::get<IndustryObject>(industry.objectId);
            if (!industryObj->hasFlags(IndustryObjectFlags::requiresElectricityPylons))
            {
                continue;
            }

            auto numTownsLeft = 3;
            for (auto& town : TownManager::towns())
            {
                // Figure out distance and rotation (angle) between town and industry
                auto rotationBL = 2;
                auto xDist = town.x - industry.x; // ax
                if (xDist < 0)
                {
                    xDist = -xDist;
                    rotationBL = 0;
                }

                auto rotationBH = 1;
                auto yDist = town.y - industry.y; // dx
                if (yDist < 0)
                {
                    yDist = -yDist;
                    rotationBH = 3;
                }

                auto manhattanDistance = xDist + yDist; // bp
                if (manhattanDistance < 800 || manhattanDistance > 2240)
                {
                    continue;
                }

                // Order distances such that the longest edge is in xDist
                if (yDist > xDist)
                {
                    std::swap(xDist, yDist);
                    std::swap(rotationBL, rotationBH);
                }

                auto origin = World::Pos2(industry.x, industry.y);
                const auto lineEnd = placeBuildingsAlongLine(origin, buildingObj->averageNumberOnMap, id, xDist / kTileSize, rotationBL);
                placeBuildingsAlongLine(lineEnd, buildingObj->averageNumberOnMap, id, yDist / kTileSize, rotationBH);

                // 0x0042E9FF
                numTownsLeft--;
                if (!numTownsLeft)
                {
                    break;
                }
            }
        }
    }

    // 0x0042EA29
    // Example: 'Lighthouse' building object
    static void generateMiscBuildingType2(const BuildingObject* buildingObj, const size_t id)
    {
        generateMiscBuilding(buildingObj, id, [](const Tile& tile) {
            // This kind of object (e.g. a lighthouse) needs to be around water
            return TileManager::countSurroundingWaterTiles(World::toWorldSpace(tile.pos)) >= 50;
        });
    }

    // 0x0042EB94
    // Example: 'Castle Ruins' building object
    static void generateMiscBuildingType3(const BuildingObject* buildingObj, const size_t id)
    {
        generateMiscBuilding(buildingObj, id, [](const Tile& tile) {
            auto* surface = tile.surface();
            return surface->baseZ() >= 40 && surface->baseZ() <= 92;
        });
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

    // 0x0043C90C
    void generate(const Scenario::Options& options)
    {
        Input::processMessagesMini();

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
            HeightMap heightMap(TileManager::getMapColumns(), TileManager::getMapRows());

            generateHeightMap(options, heightMap);
            updateProgress(25);

            generateRivers(options, heightMap);
            updateProgress(30);

            generateLand(heightMap);
            updateProgress(35);

            generateWater(heightMap);
            updateProgress(45);

            generateTerrain(heightMap);
            updateProgress(55);
        }

        generateSurfaceVariation();
        updateProgress(175);

        generateTrees();
        updateProgress(200);

        generateTowns();
        updateProgress(225);

        generateIndustries(225, 245);
        updateProgress(245);

        generateMiscBuildings();
        updateProgress(250);

        generateSurfaceVariation();
        updateProgress(255);

        Scenario::sub_4969E0(0);
        Scenario::sub_4748D4();
        Ui::ProgressBar::end();
    }

    void setPngHeightmapPath(const fs::path& path)
    {
        _pngHeightmapPath = path;
    }

    fs::path getPngHeightmapPath()
    {
        return _pngHeightmapPath;
    }
}
