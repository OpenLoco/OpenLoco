#include "ScaffoldingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x0042DF15
    void ScaffoldingObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::dark_olive_green);

        Gfx::drawImage(&dpi, x, y + 23, colourImage + 24);
        Gfx::drawImage(&dpi, x, y + 23, colourImage + 25);
        Gfx::drawImage(&dpi, x, y + 23, colourImage + 27);
    }
}
