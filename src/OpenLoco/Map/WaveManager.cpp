#include "WaveManager.h"
#include "../Core/LocoFixedVector.hpp"
#include "../Interop/Interop.hpp"
#include "../Ui/WindowManager.h"
#include "../Utility/Prng.hpp"
#include "../ViewportManager.h"
#include "TileManager.h"

namespace OpenLoco::Map::WaveManager
{
    using namespace OpenLoco::Interop;
    using namespace OpenLoco::Ui;

#pragma pack(push, 1)
    struct Wave
    {
        Map::Pos2 loc;  // 0x00
        uint16_t frame; // 0x04
        bool empty() const
        {
            return loc.x == Location::null;
        }
    };
#pragma pack(pop)

    static loco_global<Wave[64], 0x009586DC> _waves;
    static loco_global<Utility::prng, 0x00525E20> _prng; // not the gPrng

    const static Pos2 _offsets[4] = {
        Pos2(+32, 0),
        Pos2(-32, 0),
        Pos2(0, +32),
        Pos2(0, -32),
    };

    static LocoFixedVector<Wave> waves()
    {
        return LocoFixedVector<Wave>(_waves);
    }

    // 0x0046959C
    void createWave(SurfaceElement& surface, int16_t x, int16_t y, uint8_t waveIndex)
    {
        auto coord2D = gameToScreen(Pos3(x + 16, y + 16, surface.water() * 16), WindowManager::getCurrentRotation());
        auto w = WindowManager::findWindowShowing(coord2D);
        if (w == nullptr)
            return;

        uint16_t dx2 = _prng->randNext() & 0xFFFF;
        if (dx2 > 0x1745)
            return;

        // Check whether surrounding tiles are water
        for (const auto& offset : _offsets)
        {
            if (x + offset.x > 0x2FFF)
                return;
            if (y + offset.y > 0x2FFF)
                return;
            const auto tile = TileManager::get(x + offset.x, y + offset.y);
            if (tile.isNull())
                return;
            const auto* nearbySurface = tile.surface();
            if (nearbySurface == nullptr)
                return;
            if (nearbySurface->water() == 0)
                return;
        }

        _waves[waveIndex].loc = Map::Pos2(x, y);
        _waves[waveIndex].frame = 0;
        surface.setFlag6(true);

        ViewportManager::invalidate({ x, y }, surface.water() * 16, surface.water() * 16, ZoomLevel::full);
    }

    // 0x004C56F6
    void update()
    {
        if (!(addr<0x00525E28, uint32_t>() & 1) || (scenarioTicks() & 0x3))
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
        for (auto& wave : _waves)
        {
            wave.loc.x = Location::null;
        }
    }
}
