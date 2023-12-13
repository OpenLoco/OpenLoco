#include "PaintWall.h"
#include "Graphics/Colour.h"
#include "Map/WallElement.h"
#include "Paint.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Paint
{
    // 0x004C3D7C
    void paintWall([[maybe_unused]] PaintSession& session, World::WallElement& elWall)
    {
        registers regs;
        regs.esi = X86Pointer(&elWall);
        regs.ecx = (session.getRotation() + (elWall.data()[0] & 0x3)) & 0x3;
        regs.dx = elWall.baseHeight();
        call(0x004C3D7C, regs);
    }
}
