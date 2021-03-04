#include "Thing.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Ptr.h"
#include "../ViewportManager.h"
#include <algorithm>

using namespace OpenLoco;
using namespace OpenLoco::Interop;

// 0x0046FC83
void thing_base::moveTo(loc16 loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = ToInt(this);
    call(0x0046FC83, regs);
}

// 0x004CBB01
void OpenLoco::thing_base::invalidateSprite()
{
    Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
}
