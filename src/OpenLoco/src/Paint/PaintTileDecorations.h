#pragma once
#include <OpenLoco/Engine/Map.hpp>
#include <cstdint>

namespace OpenLoco::Paint
{
    // Base height offset for decorations such as direction arrows and height markers
    int8_t getTrackDecorationHeightOffset(bool isFirstTile, uint8_t trackId);

    uint32_t getHeightMarkerImage(const coord_t height);
}
