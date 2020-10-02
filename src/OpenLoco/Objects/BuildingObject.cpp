#include "BuildingObject.h"
#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    void building_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
    }
    // 0x0042DB95
    void building_object::drawBuilding(Gfx::drawpixelinfo_t* clipped, uint8_t buildingRotation, int16_t x, int16_t y, colour_t colour)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = colour;
        regs.eax = buildingRotation;
        regs.edi = (int32_t)clipped;
        regs.ebp = (uint32_t)this;
        call(0x0042DB95, regs);
    }
}
