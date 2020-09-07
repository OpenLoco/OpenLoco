#include "thing.h"
#include "../Config.h"
#include "../Graphics/gfx.h"
#include "../ViewportManager.h"
#include "../interop/interop.hpp"
#include <algorithm>

using namespace openloco;
using namespace openloco::interop;

// 0x0046FC83
void thing_base::moveTo(loc16 loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = (int32_t)this;
    call(0x0046FC83, regs);
}

// 0x004CBB01
void openloco::thing_base::invalidateSprite()
{
    ui::viewportmgr::invalidate((Thing*)this, ZoomLevel::eighth);
}
