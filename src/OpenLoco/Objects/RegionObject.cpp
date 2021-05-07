#include "RegionObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x0043CB93
    void RegionObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&context, x, y, image);
    }
}
