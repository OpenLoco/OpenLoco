#include "RoadStationObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00490C17
    void RoadStationObject::drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        Gfx::drawImage(&dpi, x - 34, y - 34, colourImage);

        auto colour = PaletteIndex::index_3B;
        if (!(flags & RoadStationFlags::recolourable))
        {
            colour = PaletteIndex::index_2E;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        Gfx::drawImage(&dpi, x - 34, y - 34, translucentImage);
    }

    // 0x00490C59
    void RoadStationObject::drawDescription(Gfx::Context& dpi, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Gfx::point_t rowPosition = { x, y };
        ObjectManager::drawGenericDescription(dpi, rowPosition, designed_year, obsolete_year);
    }
}
