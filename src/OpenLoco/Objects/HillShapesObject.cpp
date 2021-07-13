#include "HillShapesObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00463BBD
    void HillShapesObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto imageId = image + hillHeightMapCount + mountainHeightMapCount;

        Gfx::drawImage(&context, x, y, imageId);
    }
}
