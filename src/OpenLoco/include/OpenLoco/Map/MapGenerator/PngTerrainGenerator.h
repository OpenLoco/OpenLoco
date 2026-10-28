#pragma once

#include "HeightMap.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::Gfx
{
    class PngImage;
}

namespace OpenLoco::Scenario
{
    struct Options;
}

namespace OpenLoco::World::MapGenerator
{
    class PngTerrainGenerator
    {
    public:
        // The map size to use for a heightmap image: the native size of the image if that is a valid map
        // size, otherwise defaultSize (the image is then scaled to the map, see generate).
        static TilePos2 getMapSizeForImage(const Gfx::PngImage& image, TilePos2 defaultSize);

        void generate(const Scenario::Options& options, const Gfx::PngImage* pngImage, HeightMap& heightMap);
    };
}
