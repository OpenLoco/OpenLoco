#include "../IndustryManager.h"
#include "../Interop/Interop.hpp"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "Tile.h"
#include "TileManager.h"

using namespace OpenLoco;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

#pragma pack(push, 1)
struct unk1
{
    uint16_t x;     // 0x00
    uint16_t y;     // 0x02
    uint16_t frame; // 0x04
};
#pragma pack(pop)

static loco_global<Utility::prng, 0x00525E20> _prng;
static loco_global<unk1[64], 0x009586DC> _9586DC;
static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;

static Pos2 _offsets[4] = {
    Pos2(+32, 0),
    Pos2(-32, 0),
    Pos2(0, +32),
    Pos2(0, -32),
};

// 0x0046959C
void Map::SurfaceElement::createWave(int16_t x, int16_t y, int animationIndex)
{
    auto coord2D = gameToScreen(Pos3(x + 16, y + 16, this->water() * 16), gCurrentRotation);
    auto w = WindowManager::findWindowShowing(coord2D);
    if (w == nullptr)
        return;

    uint16_t dx2 = _prng->randNext() & 0xFFFF;
    if (dx2 > 0x1745)
        return;

    // Check whether surrounding tiles are water
    for (auto offset : _offsets)
    {
        if (x + offset.x > 0x2FFF)
            return;
        if (y + offset.y > 0x2FFF)
            return;
        auto tile = Map::TileManager::get(x + offset.x, y + offset.y);
        if (tile.isNull())
            return;
        auto surface = tile.surface();
        if (surface->water() == 0)
            return;
    }

    _9586DC[animationIndex].x = x;
    _9586DC[animationIndex].y = y;
    _9586DC[animationIndex].frame = 0;
    this->setFlag6(true);

    ViewportManager::invalidate({ x, y }, this->water() * 16, this->water() * 16, ZoomLevel::full);
}
