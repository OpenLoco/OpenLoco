#include "Map/StationElement.h"
#include "Paint.h"
#include "PaintStation.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::Paint
{
    // 0x0048B86E
    void paintDocks(PaintSession& session, const World::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B86E, regs);
    }
}
