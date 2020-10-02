#include "WaterObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void water_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y)
    {
        auto colourImage = Gfx::recolourTranslucent(Gfx::recolour(image + 35), 32);
        Gfx::drawImage(&dpi, x, y, colourImage);
        Gfx::drawImage(&dpi, x, y, image + 30);
    }
}
