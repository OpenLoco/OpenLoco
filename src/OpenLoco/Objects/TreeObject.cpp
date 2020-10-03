#include "TreeObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static const Gfx::point_t objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };

    // 0x004BE2A2
    void tree_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;

        xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        if (Gfx::clipDrawpixelinfo(&clipped, &dpi, pos, objectPreviewSize))
        {
            uint32_t image = treeGrowth[growth] * num_rotations;
            auto rotation = (num_rotations - 1) & 2;
            image += rotation;
            image += sprites[season_state];

            auto colourOptions = colours;
            if (colourOptions != 0)
            {

                colour_t colour = Utility::bitScanReverse(colourOptions);

                if (colour == 0xFF)
                {
                    colour = 0;
                }

                image = Gfx::recolour(image, colour);
            }

            Gfx::point_t treePos = { objectPreviewOffset.x, 104 };
            if (var_08 & (1 << 0))
            {
                auto snowImage = treeGrowth[growth] * num_rotations;
                snowImage += rotation;
                snowImage += sprites[season_state + 6];

                if (colourOptions != 0)
                {

                    colour_t colour = Utility::bitScanReverse(colourOptions);

                    if (colour == 0xFF)
                    {
                        colour = 0;
                    }

                    snowImage = Gfx::recolour(snowImage, colour);
                }
                treePos.x = 84;
                Gfx::drawImage(clipped, treePos.x, treePos.y, snowImage);
                treePos.x = 28;
            }
            Gfx::drawImage(clipped, treePos.x, treePos.y, image);
        }
    }
}
