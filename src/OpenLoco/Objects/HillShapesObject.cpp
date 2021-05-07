#include "HillShapesObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00463BBD
    void HillShapesObject::drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const
    {
        auto imageId = image + hillHeightMapCount + mountainHeightMapCount;

        Gfx::drawImage(&dpi, x, y, imageId);
    }
}
