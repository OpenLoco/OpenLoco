#include "TreeObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004BE2A2
    void TreeObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        uint32_t image = treeGrowth[growth] * num_rotations;
        auto rotation = (num_rotations - 1) & 2;
        image += rotation;
        image += sprites[season_state];

        auto colourOptions = colours;
        if (colourOptions != 0)
        {

            Colour_t colour = Utility::bitScanReverse(colourOptions);

            if (colour == 0xFF)
            {
                colour = 0;
            }

            image = Gfx::recolour(image, colour);
        }

        Gfx::point_t treePos = Gfx::point_t{ x, y } + Gfx::point_t{ 0, 48 };

        if (var_08 & (1 << 0))
        {
            auto snowImage = treeGrowth[growth] * num_rotations;
            snowImage += rotation;
            snowImage += sprites[season_state + 6];

            if (colourOptions != 0)
            {

                Colour_t colour = Utility::bitScanReverse(colourOptions);

                if (colour == 0xFF)
                {
                    colour = 0;
                }

                snowImage = Gfx::recolour(snowImage, colour);
            }
            treePos.x = x + 28;
            Gfx::drawImage(&dpi, treePos.x, treePos.y, snowImage);
            treePos.x = 28;
        }
        Gfx::drawImage(&dpi, treePos.x, treePos.y, image);
    }
}
