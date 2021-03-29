#include "RoadExtraObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00477EB9
    void RoadExtraObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        Gfx::drawImage(&dpi, x, y, colourImage + 36);
        Gfx::drawImage(&dpi, x, y, colourImage + 37);
        Gfx::drawImage(&dpi, x, y, colourImage);
        Gfx::drawImage(&dpi, x, y, colourImage + 33);
        Gfx::drawImage(&dpi, x, y, colourImage + 32);
    }
}
