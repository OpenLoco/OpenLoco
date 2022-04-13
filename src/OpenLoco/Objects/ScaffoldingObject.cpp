#include "ScaffoldingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x0042DF15
    void ScaffoldingObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour2::yellow);

        Gfx::drawImage(&context, x, y + 23, colourImage + 24);
        Gfx::drawImage(&context, x, y + 23, colourImage + 25);
        Gfx::drawImage(&context, x, y + 23, colourImage + 27);
    }
}
