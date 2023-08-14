#include "OriginalTerrainGenerator.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "MapGenerator.h"
#include "Objects/HillShapesObject.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"

namespace OpenLoco::World::MapGenerator
{
    void OriginalTerrainGenerator::blitHill(const S5::Options& options, HeightMap& heightMap)
    {
        const uint32_t randomVal = getGameState().rng.randNext();
        const bool flipHillImageLateral = randomVal & 0x100;
        const bool flipHillImageVertical = randomVal & 0x200;

        const auto hillShapesObj = ObjectManager::get<HillShapesObject>();
        const uint8_t hillShapeCount = hillShapesObj->hillHeightMapCount + hillShapesObj->mountainHeightMapCount;

        // The hill index calculation is a minor simpliciation compared to vanilla
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
        const auto randX = (randomVal >> 14) & 0x1FF;
        const auto randY = (randomVal >> 23) & 0x1FF;
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
            if ((randX + featureWidth >= 382) || (randY + featureHeight >= 382))
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
            int32_t y = (featureHeight + randY - 1) & 0x1FF;
            for (auto i = 0; i < featureHeight; ++i)
            {
                const auto data = *src++;
                heightMap[Point{ x, y }] = std::max(data, heightMap[Point{ x, y }]);
                y--;
                y &= 0x1FF;
            }
            x++;
            x &= 0x1FF;
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
                heightMap[Point{ x, y }] = std::max(data, heightMap[Point{ x, y }]);
                y++;
                y &= 0x1FF;
            }
            x++;
            x &= 0x1FF;
        }
    }

    void OriginalTerrainGenerator::blitImageNormalXFlippedY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY)
    {
        auto* src = g1Element->offset;
        int32_t y = randY;
        for (auto j = 0; j < featureHeight; ++j)
        {
            int32_t x = (randX + featureWidth - 1) & 0x1FF;
            for (auto i = 0; i < featureWidth; ++i)
            {
                const auto data = *src++;
                heightMap[Point{ x, y }] = std::max(data, heightMap[Point{ x, y }]);
                x--;
                x &= 0x1FF;
            }
            y++;
            y &= 0x1FF;
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

                heightMap[Point{ x, y }] = std::max(data, heightMap[Point{ x, y }]);

                x++;
                x &= 0x1FF;
            }
            y++;
            y &= 0x1FF;
        }
    }

    // 0x00462518
    void OriginalTerrainGenerator::generateHills(const S5::Options& options, HeightMap& heightMap)
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
        auto* dst = heightMap.data();

        for (auto y = World::kMapRows; y > 0; y--)
        {
            dst += World::kMapColumns;

            for (auto x = World::kMapColumns; x > 0; x--)
            {
                dst--;
                *dst = std::max<uint8_t>(*dst, *src);
                src++;
            }

            src += World::kMapPitch;
        }
    }

    // 0x00462590
    void OriginalTerrainGenerator::capSeaLevels(HeightMap& heightMap)
    {
        auto seaLevel = getGameState().seaLevel;
        auto* dst = heightMap.data();

        for (auto i = World::kMapPitch * (World::kMapPitch - 1) - 1; i > 0; i--)
        {
            if (seaLevel != *(dst))
                continue;

            if (seaLevel != *(dst + 1))
                continue;

            if (seaLevel != *(dst + World::kMapPitch))
                continue;

            if (seaLevel != *(dst + World::kMapPitch + 1))
                continue;

            *dst += 1;
            *(dst + 1) += 1;
            *(dst + World::kMapPitch) += 1;
            *(dst + World::kMapPitch + 1) += 1;

            dst++;
        }
    }

    void OriginalTerrainGenerator::generate(const S5::Options& options, HeightMap& heightMap)
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
