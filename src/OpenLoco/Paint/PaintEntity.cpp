#include "PaintEntity.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    // 0x0046FA88
    void paintEntities(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x0046FA88, regs);
    }

    // 0x0046FB67
    void paintEntities2(PaintSession& session, const Map::map_pos& loc)
    {
        registers regs{};
        regs.eax = loc.x;
        regs.ecx = loc.y;
        call(0x0046FB67, regs);
    }
}
