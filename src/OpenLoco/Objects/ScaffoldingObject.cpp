#include "ScaffoldingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static const xy32 objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };

    // 0x0042DF15
    void scaffolding_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;

        xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        if (Gfx::clipDrawpixelinfo(&clipped, &dpi, pos, objectPreviewSize))
        {
            auto colourImage = Gfx::recolour(image, Colour::dark_olive_green);

            Gfx::drawImage(&dpi, objectPreviewOffset.x, 79, colourImage + 24);
            Gfx::drawImage(&dpi, objectPreviewOffset.x, 79, colourImage + 25);
            Gfx::drawImage(&dpi, objectPreviewOffset.x, 79, colourImage + 27);
        }
    }
}
