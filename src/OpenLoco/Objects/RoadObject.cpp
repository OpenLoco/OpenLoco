#include "RoadObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00477DFE
    void RoadObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);
        if (var_24 == 1)
        {
            Gfx::drawImage(&dpi, x, y, colourImage + 34);
            Gfx::drawImage(&dpi, x, y, colourImage + 36);
            Gfx::drawImage(&dpi, x, y, colourImage + 38);
        }
        else
        {
            Gfx::drawImage(&dpi, x, y, colourImage + 34);
        }
    }
}
