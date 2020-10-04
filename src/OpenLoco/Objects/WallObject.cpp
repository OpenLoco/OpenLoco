#include "WallObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004C4B0B
    void wall_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto image = sprite;
        image = Gfx::recolour(image, Colour::salmon_pink);
        if (flags & (1 << 6))
        {
            image |= (1 << 31) | (1 << 28);
        }

        Gfx::drawImage(&dpi, x + 14, y + 16 + (var_08 * 2), image);
        if (flags & (1 << 1))
        {
            Gfx::drawImage(&dpi, x + 14, y + 16 + (var_08 * 2), Gfx::recolourTranslucent(sprite + 6, 140));
        }
        else
        {
            if (flags & (1 << 4))
            {
                Gfx::drawImage(&dpi, x + 14, y + 16 + (var_08 * 2), image + 1);
            }
        }
    }
}
