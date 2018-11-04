#include "../industrymgr.h"
#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"
#include "../viewportmgr.h"
#include "tile.h"
#include "tilemgr.h"

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::ui;

#pragma pack(push, 1)
struct unk1
{
    uint16_t x;     // 0x00
    uint16_t y;     // 0x02
    uint16_t frame; // 0x04
};
#pragma pack(pop)

static loco_global<utility::prng, 0x00525E20> _prng;
static loco_global<unk1[64], 0x009586DC> _9586DC;
static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;

static map_pos _offsets[4] = {
    map_pos(+32, 0),
    map_pos(-32, 0),
    map_pos(0, +32),
    map_pos(0, -32),
};

// 0x0046959C
void map::surface_element::createWave(int16_t x, int16_t y, int animationIndex)
{
    auto coord2D = coordinate_3d_to_2d(x + 16, y + 16, this->water() * 16, gCurrentRotation);
    auto w = WindowManager::findWindowShowing(coord2D);
    if (w == nullptr)
        return;

    uint16_t dx2 = _prng->rand_next() & 0xFFFF;
    if (dx2 > 0x1745)
        return;

    // Check whether surrounding tiles are water
    for (auto offset : _offsets)
    {
        if (x + offset.x > 0x2FFF)
            return;
        if (y + offset.y > 0x2FFF)
            return;
        auto tile = map::tilemgr::get(x + offset.x, y + offset.y);
        if (tile.is_null())
            return;
        auto surface = tile.surface();
        if (surface->water() == 0)
            return;
    }

    _9586DC[animationIndex].x = x;
    _9586DC[animationIndex].y = y;
    _9586DC[animationIndex].frame = 0;
    this->set_flag_6();

    viewportmgr::invalidate({ x, y }, this->water() * 16, this->water() * 16, viewportmgr::ZoomLevel::full);
}
