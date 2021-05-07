#include "TrackExtraObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004A6D5F
    void TrackExtraObject::drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        if (is_overhead == 0)
        {
            Gfx::drawImage(&dpi, x, y, colourImage);
        }
        else
        {
            Gfx::drawImage(&dpi, x, y, colourImage);
            Gfx::drawImage(&dpi, x, y, colourImage + 97);
            Gfx::drawImage(&dpi, x, y, colourImage + 96);
        }
    }
}
