#include "TunnelObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00469806
    void TunnelObject::drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&dpi, x - 16, y + 15, image);
        Gfx::drawImage(&dpi, x - 16, y + 15, image + 1);
    }
}
