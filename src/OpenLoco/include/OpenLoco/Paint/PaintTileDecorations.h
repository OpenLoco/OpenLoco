#pragma once
#include <OpenLoco/Engine/World.hpp>
#include <cstdint>

namespace OpenLoco::Paint
{
    // Base height offset for decorations such as direction arrows and height markers
    int8_t getTrackDecorationHeightOffset(bool isFirstTile, uint8_t trackId);
    int8_t getRoadDecorationHeightOffset(const bool isFirstTile, const uint8_t roadId);

    uint32_t getHeightMarkerImage(const coord_t height);
}
