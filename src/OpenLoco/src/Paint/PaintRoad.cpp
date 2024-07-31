#include "PaintRoad.h"
#include "Map/RoadElement.h"
#include "Paint.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::Paint
{
    // 0x004759A6
    void paintRoad(PaintSession& session, const World::RoadElement& elRoad)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elRoad);
        regs.ecx = (session.getRotation() + elRoad.rotation()) & 0x3;
        regs.dx = elRoad.baseHeight();
        Interop::call(0x004759A6, regs);
    }
}
