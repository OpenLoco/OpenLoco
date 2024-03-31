#pragma once

#include "HeightMap.h"
#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::S5
{
    struct Options;
}

namespace OpenLoco::World::MapGenerator
{
    class PngTerrainGenerator
    {
    public:
        void generate(const S5::Options& options, const fs::path& path, HeightMap& heightMap);
    };
}
