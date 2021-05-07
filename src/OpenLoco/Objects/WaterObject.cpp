#include "WaterObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004C56D3
    void WaterObject::drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolourTranslucent(Gfx::recolour(image + 35), 32);
        Gfx::drawImage(&dpi, x, y, colourImage);
        Gfx::drawImage(&dpi, x, y, image + 30);
    }
}
