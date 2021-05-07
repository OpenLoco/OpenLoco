#include "RockObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x00469A06
    void RockObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&context, x - 30, y, image);
        Gfx::drawImage(&context, x - 30, y, image + 16);
    }
}
