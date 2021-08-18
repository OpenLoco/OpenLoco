#include "TreeObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004BE2A2
    void TreeObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        uint32_t image = treeGrowth[growth] * num_rotations;
        auto rotation = (num_rotations - 1) & 2;
        image += rotation;
        image += sprites[0][season_state];

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

        Ui::Point treePos = Ui::Point{ x, y } + Ui::Point{ 0, 48 };

        if (flags & TreeObjectFlags::hasSnowVariation)
        {
            auto snowImage = treeGrowth[growth] * num_rotations;
            snowImage += rotation;
            snowImage += sprites[1][season_state];

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
            Gfx::drawImage(&context, treePos.x, treePos.y, snowImage);
            treePos.x = 28;
        }
        Gfx::drawImage(&context, treePos.x, treePos.y, image);
    }
}
