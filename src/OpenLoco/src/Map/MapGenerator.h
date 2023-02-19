#pragma once
#include <cstdint>
#include <optional>

namespace OpenLoco::S5
{
    struct Options;
}

namespace OpenLoco::World
{
    struct SurfaceElement;
}

namespace OpenLoco::World::MapGenerator
{
    void generate(const S5::Options& options);
    std::optional<uint8_t> getRandomTerrainVariation(const SurfaceElement& surface);
}
