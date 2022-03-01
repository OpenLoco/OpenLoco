#pragma once
#include "../Core/Optional.hpp"
#include <cstdint>

namespace OpenLoco::S5
{
    struct Options;
}

namespace OpenLoco::Map
{
    struct SurfaceElement;
}

namespace OpenLoco::Map::MapGenerator
{
    void generate(const S5::Options& options);
    std::optional<uint8_t> getRandomTerrainVariation(const SurfaceElement& surface);
}
