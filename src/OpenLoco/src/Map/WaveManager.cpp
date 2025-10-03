#include "WaveManager.h"
#include "Engine/Limits.h"
#include "Game.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Random.h"
#include "ScenarioManager.h"
#include "SurfaceElement.h"
#include "TileManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "Wave.h"
#include <OpenLoco/Core/LocoFixedVector.hpp>
#include <OpenLoco/Core/Prng.h>
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::World::WaveManager
{
    using namespace OpenLoco::Interop;
    using namespace OpenLoco::Ui;

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
    void createWave(SurfaceElement& surface, const World::Pos2& pos)
    {
        const auto waveIndex = getWaveIndex(World::toTileSpace(pos));
        auto& wave = rawWaves()[waveIndex];
        if (!wave.empty())
        {
            return;
        }
        auto vpPoint = gameToScreen(Pos3(pos.x + 16, pos.y + 16, surface.waterHeight()), WindowManager::getCurrentRotation());
        auto w = WindowManager::findWindowShowing(vpPoint);
        if (w == nullptr)
        {
            return;
        }

        uint16_t dx2 = gPrng2().randNext() & 0xFFFF;
        if (dx2 > 0x1745)
        {
            return;
        }

        // Check whether surrounding tiles are water
        for (const auto& offset : _offsets)
        {
            auto searchLoc = pos + offset;
            if (!World::TileManager::validCoords(searchLoc))
            {
                return;
            }

            const auto tile = TileManager::get(searchLoc);
            if (tile.isNull())
            {
                return;
            }
            const auto* nearbySurface = tile.surface();
            if (nearbySurface == nullptr)
            {
                return;
            }
            if (nearbySurface->water() == 0)
            {
                return;
            }
        }

        wave.loc = pos;
        wave.frame = 0;
        surface.setFlag6(true);

        ViewportManager::invalidate(pos, surface.waterHeight(), surface.waterHeight(), ZoomLevel::full);
    }

    const Wave& getWave(const uint8_t waveIndex)
    {
        return rawWaves()[waveIndex];
    }

    // 0x004C56F6
    void update()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded) || (ScenarioManager::getScenarioTicks() & 0x3))
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

            ViewportManager::invalidate(wave.loc, surface->waterHeight(), surface->waterHeight(), ZoomLevel::full);

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
