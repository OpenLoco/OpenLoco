#pragma once
#include "Tile.h"

namespace OpenLoco::World::WaveManager
{
    void update();
    void reset();
    void createWave(SurfaceElement& surface, const World::Pos2& pos);

    constexpr uint8_t getWaveIndex(const World::TilePos2& pos)
    {
        return (pos.x & 0x7) | ((pos.y & 0x7) << 3);
    }
}
