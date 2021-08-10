#pragma once
#include "Tile.h"

namespace OpenLoco::Map::WaveManager
{
    void update();
    void reset();
    void createWave(SurfaceElement& surface, int16_t x, int16_t y, uint8_t waveIndex);

    constexpr uint8_t getWaveIndex(const Map::TilePos2& pos)
    {
        return (pos.x & 0x7) | ((pos.y & 0x7) << 3);
    }
}
