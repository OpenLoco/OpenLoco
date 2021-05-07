#include "SnowObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00469A75
    void SnowObject::drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&dpi, x, y, image);
    }
}
