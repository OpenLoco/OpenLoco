#include "WallObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004C4B0B
    void WallObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto image = sprite;
        image = Gfx::recolour(image, Colour::mutedDarkRed);
        if (flags & (1 << 6))
        {
            image |= (1 << 31) | (1 << 28);
        }

        Gfx::drawImage(&context, x + 14, y + 16 + (var_08 * 2), image);
        if (flags & (1 << 1))
        {
            Gfx::drawImage(&context, x + 14, y + 16 + (var_08 * 2), Gfx::recolourTranslucent(sprite + 6, 140));
        }
        else
        {
            if (flags & (1 << 4))
            {
                Gfx::drawImage(&context, x + 14, y + 16 + (var_08 * 2), image + 1);
            }
        }
    }
}
