#include "BuildingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Ptr.h"
#include "ObjectManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x0042DE40
    void building_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        colour_t colour = Utility::bitScanReverse(colours);

        if (colour == 0xFF)
        {
            colour = 0;
        }

        drawBuilding(&dpi, 1, x, y + 40, colour);
    }

    // 0x0042DB95
    void building_object::drawBuilding(Gfx::drawpixelinfo_t* clipped, uint8_t buildingRotation, int16_t x, int16_t y, colour_t colour) const
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = colour;
        regs.eax = buildingRotation;
        regs.edi = ToInt(clipped);
        regs.ebp = ToInt(this);
        call(0x0042DB95, regs);
    }

    // 0x0042DE82
    void building_object::drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Gfx::point_t rowPosition = { x, y };
        ObjectManager::drawGenericDescription(dpi, rowPosition, designedYear, obsoleteYear);
    }
}
