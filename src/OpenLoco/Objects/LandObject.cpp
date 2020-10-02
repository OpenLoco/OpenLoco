#include "LandObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void land_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        uint32_t imageId = image + (var_03 - 1) * var_0E;
        Gfx::drawImage(&dpi, x, y, imageId);
    }
}
