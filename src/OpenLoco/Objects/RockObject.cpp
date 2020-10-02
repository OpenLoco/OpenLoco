#include "RockObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void rock_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y)
    {
        Gfx::drawImage(&dpi, x - 30, y, image);
        Gfx::drawImage(&dpi, x - 30, y, image + 16);
    }
}
