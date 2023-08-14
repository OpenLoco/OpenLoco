#pragma once

#include "HeightMap.h"
#include "Types.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace OpenLoco::S5
{
    struct Options;
}

namespace OpenLoco::Gfx
{
    struct G1Element;
}

namespace OpenLoco::World::MapGenerator
{
    class OriginalTerrainGenerator
    {
    private:
        // 0x004626B7
        void blitHill(const S5::Options& options, HeightMap& heightMap);

        static void blitImageFlippedXFlippedY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY);

        static void blitImageFlippedXNormalY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY);

        static void blitImageNormalXFlippedY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY);

        static void blitImageNormalXNormalY(Gfx::G1Element* g1Element, uint8_t featureWidth, uint8_t featureHeight, HeightMap& heightMap, tile_coord_t randX, tile_coord_t randY);

        // 0x00462518
        void generateHills(const S5::Options& options, HeightMap& heightMap);

        // 0x00462556
        void copyHeightMapFromG1(Gfx::G1Element* g1Element, HeightMap& heightMap);

        // 0x00462590
        void capSeaLevels(HeightMap& heightMap);

    public:
        // 0x004624F0
        void generate(const S5::Options& options, HeightMap& heightMap);
    };
}
