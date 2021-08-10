#pragma once
#include "Tile.h"

namespace OpenLoco::Map::WaveManager
{
    void update();
    void reset();
    void createWave(SurfaceElement& surface, int16_t x, int16_t y, int32_t animationIndex);
}
