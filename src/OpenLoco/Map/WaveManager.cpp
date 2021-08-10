#include "WaveManager.h"
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
        uint16_t x;     // 0x00
        uint16_t y;     // 0x02
        uint16_t frame; // 0x04
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

    // 0x0046959C
    void createWave(SurfaceElement& surface, int16_t x, int16_t y, int32_t animationIndex)
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

        _waves[animationIndex].x = x;
        _waves[animationIndex].y = y;
        _waves[animationIndex].frame = 0;
        surface.setFlag6(true);

        ViewportManager::invalidate({ x, y }, surface.water() * 16, surface.water() * 16, ZoomLevel::full);
    }
}
