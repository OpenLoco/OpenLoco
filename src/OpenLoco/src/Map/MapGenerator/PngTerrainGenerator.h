#pragma once

#include "HeightMap.h"
#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco
{
    struct Options;
}

namespace OpenLoco::World::MapGenerator
{
    class PngTerrainGenerator
    {
    public:
        void generate(const Options& options, const fs::path& path, HeightMap& heightMap);
    };
}
