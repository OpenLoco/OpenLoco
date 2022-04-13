#include "RoadExtraObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00477EB9
    void RoadExtraObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour2::mutedDarkRed);

        Gfx::drawImage(&context, x, y, colourImage + 36);
        Gfx::drawImage(&context, x, y, colourImage + 37);
        Gfx::drawImage(&context, x, y, colourImage);
        Gfx::drawImage(&context, x, y, colourImage + 33);
        Gfx::drawImage(&context, x, y, colourImage + 32);
    }
}
