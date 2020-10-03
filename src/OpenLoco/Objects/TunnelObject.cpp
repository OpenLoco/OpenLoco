#include "TunnelObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void tunnel_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&dpi, x - 16, y + 15, image);
        Gfx::drawImage(&dpi, x - 16, y + 15, image + 1);
    }
}
