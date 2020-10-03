#include "SnowObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void snow_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&dpi, x, y, image);
    }
}
