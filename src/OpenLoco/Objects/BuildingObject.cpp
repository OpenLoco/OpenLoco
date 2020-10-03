#include "BuildingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "ObjectManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static const xy32 objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };

    // 0x0042DE40
    void building_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;

        xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        if (Gfx::clipDrawpixelinfo(&clipped, &dpi, pos, objectPreviewSize))
        {
            colour_t colour = Utility::bitScanReverse(colours);

            if (colour == 0xFF)
            {
                colour = 0;
            }

            drawBuilding(clipped, 1, objectPreviewOffset.x, 96, colour);
        }
    }

    // 0x0042DB95
    void building_object::drawBuilding(Gfx::drawpixelinfo_t* clipped, uint8_t buildingRotation, int16_t x, int16_t y, colour_t colour) const
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

    // 0x0042DE82
    void building_object::drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::point_t rowPosition = { x, y };
        ObjectManager::drawGenericDescription(dpi, rowPosition, designedYear, obsoleteYear);
    }
}
