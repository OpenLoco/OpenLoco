#include "TrainStationObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00490A26
    void train_station_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        Gfx::drawImage(&dpi, x - 34, y - 34, colourImage);

        auto colour = 59;
        if (!(flags & TrainStationFlags::recolourable))
        {
            colour = 46;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        Gfx::drawImage(&dpi, x - 34, y - 34, translucentImage);
    }
}
