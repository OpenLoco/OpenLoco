#include "OriginalTerrainGenerator.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "MapGenerator.h"
#include "Objects/HillShapesObject.h"
#include "Objects/ObjectManager.h"
#include "ScenarioOptions.h"

namespace OpenLoco::World::MapGenerator
{
    void OriginalTerrainGenerator::blitHill(const Scenario::Options& options, HeightMap& heightMap)
    {
        const uint32_t randomVal = getGameState().rng.randNext();
        const bool flipHillImageLateral = randomVal & 0x100;
        const bool flipHillImageVertical = randomVal & 0x200;

        const auto hillShapesObj = ObjectManager::get<HillShapesObject>();
        const uint8_t hillShapeCount = hillShapesObj->hillHeightMapCount + hillShapesObj->mountainHeightMapCount;

        // The hill index calculation is a minor simplification compared to vanilla
        const uint8_t randomHillIndex = (randomVal & 0xFF) % hillShapeCount;

        const auto hillImage = hillShapesObj->image + randomHillIndex;
        const auto g1Element = Gfx::getG1Element(hillImage);

        auto featureWidth = g1Element->width & 0xFF;
        auto featureHeight = g1Element->height & 0xFF;
        if (flipHillImageLateral)
        {
            std::swap(featureWidth, featureHeight);
        }

        // 0x00462718
        const uint32_t randX = getGameState().rng.randNext(heightMap.width - 1);
        const uint32_t randY = getGameState().rng.randNext(heightMap.height - 1);
        if (randX < 2 || randY < 2)
        {
            return;
        }

        // Vanilla would use an `rcl` for the lower bit, getting the carry flag in.
        // We're substituting it with an extra bit of the random state.
        const bool topographyBit = randX > randY;
        const auto topographyId = (enumValue(options.topographyStyle) << 1) | (topographyBit ? 1U : 0);
        const auto topographyFlags = kTopographyStyleFlags[topographyId];

        const bool generateHills = (topographyFlags & TopographyFlags::hasHills) != TopographyFlags::none;
        const bool generateMountains = (topographyFlags & TopographyFlags::hasMountains) != TopographyFlags::none;

        const bool isValidHillIndex = generateHills && randomHillIndex < hillShapesObj->hillHeightMapCount;
        const bool isValidMountainIndex = generateMountains && randomHillIndex >= hillShapesObj->hillHeightMapCount;
        if (!(isValidHillIndex || isValidMountainIndex))
        {
            return;
        }

        // 0x0046276D

        if ((options.scenarioFlags & Scenario::ScenarioFlags::hillsEdgeOfMap) == Scenario::ScenarioFlags::none)
        {
            if ((randX + featureWidth >= heightMap.width - 2U) || (randY + featureHeight >= heightMap.height - 2U))
            {
                return;
            }
        }

        using BlitFunction = std::function<void(Gfx::G1Element*, uint8_t, uint8_t, HeightMap&, tile_coord_t, tile_coord_t)>;
        static const std::array<std::array<BlitFunction, 2>, 2> blitMethods = { {
            {
                blitImageNormalXNormalY,
                blitImageNormalXFlippedY,
            },
            {
                blitImageFlippedXNormalY,
                blitImageFlippedXFlippedY,
            },
        } };

        auto blitFeature = blitMethods[flipHillImageLateral][flipHillImageVertical];
        blitFeature(g1Element, featureWidth, featureHeight, heightMap, randX, randY);
    }

    void OriginalTerrainGenerator::blitImageFlippedXFlippedY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY)
    {
        auto* src = g1Element->offset;
        int32_t x = randX;
        for (auto j = 0; j < featureWidth; ++j)
        {
            int32_t y = (featureHeight + randY - 1) % (heightMap.height - 1);
            for (auto i = 0; i < featureHeight; ++i)
            {
                const auto data = *src++;
                heightMap[TilePos2(x, y)] = std::max(data, heightMap[TilePos2(x, y)]);
                auto mod = heightMap.height - 1;
                y = (y - 1 + mod) % mod;
            }
            x++;
            x %= heightMap.width - 1;
        }
    }

    void OriginalTerrainGenerator::blitImageFlippedXNormalY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY)
    {
        auto* src = g1Element->offset;
        int32_t x = randX;
        for (auto j = 0; j < featureWidth; ++j)
        {
            int32_t y = randY;
            for (auto i = 0; i < featureHeight; ++i)
            {
                const auto data = *src++;
                heightMap[TilePos2(x, y)] = std::max(data, heightMap[TilePos2(x, y)]);
                y++;
                y %= heightMap.height - 1;
            }
            x++;
            x %= heightMap.width - 1;
        }
    }

    void OriginalTerrainGenerator::blitImageNormalXFlippedY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY)
    {
        auto* src = g1Element->offset;
        int32_t y = randY;
        for (auto j = 0; j < featureHeight; ++j)
        {
            int32_t x = (randX + featureWidth - 1) % (heightMap.width - 1);
            for (auto i = 0; i < featureWidth; ++i)
            {
                const auto data = *src++;
                heightMap[TilePos2(x, y)] = std::max(data, heightMap[TilePos2(x, y)]);
                auto mod = heightMap.width - 1;
                x = (x - 1 + mod) % mod;
            }
            y++;
            y %= heightMap.height - 1;
        }
    }

    void OriginalTerrainGenerator::blitImageNormalXNormalY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY)
    {
        auto* src = g1Element->offset;
        int32_t y = randY;
        for (auto j = 0; j < featureHeight; ++j)
        {
            int32_t x = randX;
            for (auto i = 0; i < featureWidth; ++i)
            {
                const auto data = *src++;

                heightMap[TilePos2(x, y)] = std::max(data, heightMap[TilePos2(x, y)]);

                x++;
                x %= heightMap.width - 1;
            }
            y++;
            y %= heightMap.height - 1;
        }
    }

    // 0x00462518
    void OriginalTerrainGenerator::generateHills(const Scenario::Options& options, HeightMap& heightMap)
    {
        uint8_t baseCount = getGameState().rng.randNext() & 7;
        uint16_t numHills = baseCount + (options.hillDensity * 5) + 1;

        for (uint16_t i = 0; i < numHills; i++)
        {
            blitHill(options, heightMap);
        }
    }

    // 0x00462556
    void OriginalTerrainGenerator::copyHeightMapFromG1(Gfx::G1Element* g1Element, HeightMap& heightMap)
    {
        auto* src = g1Element->offset;

        for (auto y = heightMap.height - 1; y > 0; y--)
        {
            for (auto x = heightMap.width - 1; x > 0; x--)
            {
                auto height = std::max<uint8_t>(*src, heightMap[TilePos2(x, y)]);
                heightMap[TilePos2(x, y)] = height;
                src++;
            }

            src += TileManager::kMapPitch;
        }
    }

    // 0x00462590
    void OriginalTerrainGenerator::capSeaLevels(HeightMap& heightMap)
    {
        const auto seaLevel = getGameState().seaLevel;

        for (auto y = 0; y < heightMap.height - 2; y++)
        {
            for (auto x = 0; x < heightMap.width - 2; x++)
            {
                if (seaLevel != heightMap[TilePos2(x + 0, y + 0)])
                {
                    continue;
                }

                if (seaLevel != heightMap[TilePos2(x + 1, y + 0)])
                {
                    continue;
                }

                if (seaLevel != heightMap[TilePos2(x + 0, y + 1)])
                {
                    continue;
                }

                if (seaLevel != heightMap[TilePos2(x + 1, y + 1)])
                {
                    continue;
                }

                heightMap[TilePos2(x + 0, y + 0)] += 1;
                heightMap[TilePos2(x + 1, y + 0)] += 1;
                heightMap[TilePos2(x + 0, y + 1)] += 1;
                heightMap[TilePos2(x + 1, y + 1)] += 1;
            }
        }
    }

    void OriginalTerrainGenerator::generate(const Scenario::Options& options, HeightMap& heightMap)
    {
        std::fill_n(heightMap.data(), heightMap.size(), options.minLandHeight);

        auto hillShapesObj = ObjectManager::get<HillShapesObject>();
        if ((hillShapesObj->flags & HillShapeFlags::isHeightMap) != HillShapeFlags::none)
        {
            auto g1Element = Gfx::getG1Element(hillShapesObj->image);
            copyHeightMapFromG1(g1Element, heightMap);
        }
        else
        {
            generateHills(options, heightMap);
        }

        capSeaLevels(heightMap);
    }
}
