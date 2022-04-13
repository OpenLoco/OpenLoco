#include "TrackObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004A6CBA
    void TrackObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour2::mutedDarkRed);

        Gfx::drawImage(&context, x, y, colourImage + 18);
        Gfx::drawImage(&context, x, y, colourImage + 20);
        Gfx::drawImage(&context, x, y, colourImage + 22);
    }
}
