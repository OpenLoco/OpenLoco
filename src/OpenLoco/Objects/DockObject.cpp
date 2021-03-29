#include "DockObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00490F14
    void DockObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        Gfx::drawImage(&dpi, x - 34, y - 34, colourImage);
    }

    // 0x00490F2C
    void DockObject::drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Gfx::point_t rowPosition = { x, y };
        ObjectManager::drawGenericDescription(dpi, rowPosition, designed_year, obsolete_year);
    }
}
