#include "thing.h"
#include "../config.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../viewportmgr.h"
#include <algorithm>

using namespace openloco;
using namespace openloco::interop;

// 0x0046FC83
void thing_base::move_to(loc16 loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = (loco_ptr)this;
    call(0x0046FC83, regs);
}

// 0x004CBB01
void openloco::thing_base::invalidate_sprite()
{
    ui::viewportmgr::invalidate((Thing*)this, ui::viewportmgr::ZoomLevel::eighth);
}
