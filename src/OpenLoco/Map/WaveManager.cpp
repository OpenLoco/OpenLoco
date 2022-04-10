#include "WaveManager.h"
#include "../Core/LocoFixedVector.hpp"
#include "../Game.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"
#include "../Limits.h"
#include "../ScenarioManager.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Prng.hpp"
#include "../ViewportManager.h"
#include "TileManager.h"
#include "Wave.h"

namespace OpenLoco::Map::WaveManager
{
    using namespace OpenLoco::Interop;
    using namespace OpenLoco::Ui;

    static loco_global<Utility::prng, 0x00525E20> _prng; // not the gPrng

    const static Pos2 _offsets[4] = {
        Pos2(+32, 0),
        Pos2(-32, 0),
        Pos2(0, +32),
        Pos2(0, -32),
    };

    static auto& rawWaves()
    {
        return getGameState().waves;
    }

    static FixedVector<Wave, Limits::kMaxWaves> waves()
    {
        return FixedVector(rawWaves());
    }
    // 0x0046956E
    void createWave(SurfaceElement& surface, const Map::Pos2& pos)
    {
        const auto waveIndex = getWaveIndex(pos);
        if (!rawWaves()[getWaveIndex(pos)].empty())
        {
            return;
        }
        auto vpPoint = gameToScreen(Pos3(pos.x + 16, pos.y + 16, surface.water() * 16), WindowManager::getCurrentRotation());
        auto w = WindowManager::findWindowShowing(vpPoint);
        if (w == nullptr)
            return;

        uint16_t dx2 = _prng->randNext() & 0xFFFF;
        if (dx2 > 0x1745)
            return;

        // Check whether surrounding tiles are water
        for (const auto& offset : _offsets)
        {
            auto searchLoc = pos + offset;
            if (!Map::validCoords(searchLoc))
                return;

            const auto tile = TileManager::get(searchLoc);
            if (tile.isNull())
                return;
            const auto* nearbySurface = tile.surface();
            if (nearbySurface == nullptr)
                return;
            if (nearbySurface->water() == 0)
                return;
        }

        rawWaves()[waveIndex].loc = pos;
        rawWaves()[waveIndex].frame = 0;
        surface.setFlag6(true);

        ViewportManager::invalidate(pos, surface.water() * 16, surface.water() * 16, ZoomLevel::full);
    }

    // 0x004C56F6
    void update()
    {
        if (!Game::hasFlags(1u << 0) || (ScenarioManager::getScenarioTicks() & 0x3))
        {
            return;
        }

        for (auto& wave : waves())
        {
            auto tile = TileManager::get(wave.loc);
            auto* surface = tile.surface();
            if (surface == nullptr)
            {
                wave.loc.x = Location::null;
                continue;
            }

            ViewportManager::invalidate(wave.loc, surface->water() * 4, surface->water() * 4, ZoomLevel::full);

            if (surface->water())
            {
                wave.frame++;
                if (wave.frame < 16)
                {
                    continue;
                }
            }
            // Wave removed if 16 frames or no water
            wave.loc.x = Location::null;
            surface->setFlag6(false);
        }
    }

    // 0x004C4BC0
    void reset()
    {
        for (auto& wave : rawWaves())
        {
            wave.loc.x = Location::null;
        }
    }
}
