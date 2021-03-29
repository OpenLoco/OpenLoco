#include "RegionObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x0043CB93
    void RegionObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&dpi, x, y, image);
    }
}
