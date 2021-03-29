#include "TrackObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004A6CBA
    void TrackObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        Gfx::drawImage(&dpi, x, y, colourImage + 18);
        Gfx::drawImage(&dpi, x, y, colourImage + 20);
        Gfx::drawImage(&dpi, x, y, colourImage + 22);
    }
}
