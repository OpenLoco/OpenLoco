#pragma once

#include <cstdint>

namespace OpenLoco::World
{
    uint8_t lowerSurfaceCornerFlags(uint8_t selectedCorner, uint8_t currentSlope);
    uint8_t raiseSurfaceCornerFlags(uint8_t selectedCorner, uint8_t currentSlope);
}
