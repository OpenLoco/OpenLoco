#include "WaterObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004C56D3
    void WaterObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolourTranslucent(Gfx::recolour(image + 35), 32);
        Gfx::drawImage(&context, x, y, colourImage);
        Gfx::drawImage(&context, x, y, image + 30);
    }
}
