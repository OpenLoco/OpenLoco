#include "WallObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // TODO: These should only be defined in the ObjectSelectionWindow header file
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };
    static const xy32 objectPreviewOffset = { 56, 56 };

    // 0x004C4B0B
    void wall_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;

        xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        if (Gfx::clipDrawpixelinfo(&clipped, &dpi, pos, objectPreviewSize))
        {
            auto image = sprite;
            image = Gfx::recolour(image, Colour::salmon_pink);
            if (flags & (1 << 6))
            {
                image |= (1 << 31) | (1 << 28);
            }

            Gfx::drawImage(clipped, 70, 72 + (var_08 * 2), image);
            if (flags & (1 << 1))
            {
                Gfx::drawImage(clipped, 70, 72 + (var_08 * 2), Gfx::recolourTranslucent(sprite + 6, 140));
            }
            else
            {
                if (flags & (1 << 4))
                {
                    Gfx::drawImage(clipped, 70, 72 + (var_08 * 2), image + 1);
                }
            }
        }
    }
}
