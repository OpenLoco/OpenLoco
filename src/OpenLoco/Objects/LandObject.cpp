#include "LandObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004699A8
    void LandObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        uint32_t imageId = image + (var_03 - 1) * var_0E;
        Gfx::drawImage(&context, x, y, imageId);
    }
}
