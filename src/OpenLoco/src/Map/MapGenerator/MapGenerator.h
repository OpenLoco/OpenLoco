#pragma once

#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/FileSystem.hpp>
#include <array>
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
    enum class TopographyFlags : uint8_t
    {
        none = 0U,
        hasHills = 1U << 0,
        hasMountains = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TopographyFlags);

    // 0x004FD332
    static constexpr std::array<TopographyFlags, 10> kTopographyStyleFlags = {
        TopographyFlags::none,
        TopographyFlags::none,
        TopographyFlags::hasHills,
        TopographyFlags::hasHills,
        TopographyFlags::hasMountains | TopographyFlags::hasHills,
        TopographyFlags::hasMountains | TopographyFlags::hasHills,
        TopographyFlags::hasHills,
        TopographyFlags::hasMountains | TopographyFlags::hasHills,
        TopographyFlags::none,
        TopographyFlags::hasMountains | TopographyFlags::hasHills,
    };

    void generate(const S5::Options& options);
    std::optional<uint8_t> getRandomTerrainVariation(const SurfaceElement& surface);

    void setPngHeightmapPath(const fs::path& path);
    fs::path getPngHeightmapPath();
}
