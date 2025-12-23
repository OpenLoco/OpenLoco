#pragma once

#include "HeightMap.h"
#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::Scenario
{
    struct Options;
}

namespace OpenLoco::World::MapGenerator
{
    class PngTerrainGenerator
    {
    public:
        HeightMap generate(Scenario::Options& options, const fs::path& path);
    };
}
