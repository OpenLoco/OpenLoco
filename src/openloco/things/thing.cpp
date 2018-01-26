#include <algorithm>
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../viewportmgr.h"
#include "thing.h"

using namespace openloco;
using namespace openloco::interop;

loco_global<uint8_t, 0x0050AF25> thing_zoom_max;

// 0x0046FC83
void thing::move_to(loc16 loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = (int32_t)this;
    call(0x0046FC83, regs);
}

// 0x004CBB01
void openloco::thing::invalidate_sprite()
{
    if (sprite_left == (int16_t)0x8000u)
    {
        return;
    }

    int16_t left = sprite_left;
    int16_t top = sprite_top;
    int16_t right = sprite_right;
    int16_t bottom = sprite_bottom;
    for (auto& viewport : openloco::ui::viewportmgr::viewports())
    {
        if (viewport->zoom > thing_zoom_max)
            continue;

        if (sprite_right <= viewport->view_x)
            continue;

        if (sprite_bottom <= viewport->view_y)
            continue;

        if (sprite_left >= viewport->view_x + viewport->view_width)
            continue;

        left = std::max(sprite_left, viewport->view_x);
        right = std::min<int16_t>(sprite_right, viewport->view_x + viewport->view_width);

        if (sprite_top >= viewport->view_y + viewport->view_height)
            continue;

        bottom = std::max(sprite_bottom, viewport->view_y);
        top = std::min<int16_t>(sprite_top, viewport->view_y + viewport->view_height);

        left -= viewport->view_x;
        bottom -= viewport->view_y;
        right -= viewport->view_x;
        top -= viewport->view_y;

        left >>= viewport->zoom;
        bottom >>= viewport->zoom;
        right >>= viewport->zoom;
        top >>= viewport->zoom;

        left += viewport->x;
        bottom += viewport->y;
        right += viewport->x;
        top += viewport->y;

        openloco::gfx::set_dirty_blocks(left, top, right, bottom);
    }
}
