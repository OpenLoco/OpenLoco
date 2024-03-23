#pragma once

#include "HeightMapRange.h"
#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::World::MapGenerator
{
    class PngTerrainGenerator
    {
    public:
        void generate(const fs::path& path, HeightMapRange heightMap);
    };
}
