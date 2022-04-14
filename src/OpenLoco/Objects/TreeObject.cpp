#include "TreeObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x004BE2A2
    void TreeObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        uint32_t image = getTreeGrowthDisplayOffset() * num_rotations;
        auto rotation = (num_rotations - 1) & 2;
        image += rotation;
        image += sprites[0][season_state];

        auto colourOptions = colours;
        if (colourOptions != 0)
        {

            auto bit = Utility::bitScanReverse(colourOptions);

            auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

            image = Gfx::recolour(image, colour);
        }

        Ui::Point treePos = Ui::Point{ x, y } + Ui::Point{ 0, 48 };

        if (flags & TreeObjectFlags::hasSnowVariation)
        {
            auto snowImage = getTreeGrowthDisplayOffset() * num_rotations;
            snowImage += rotation;
            snowImage += sprites[1][season_state];

            if (colourOptions != 0)
            {
                auto bit = Utility::bitScanReverse(colourOptions);

                auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

                snowImage = Gfx::recolour(snowImage, colour);
            }
            treePos.x = x + 28;
            Gfx::drawImage(&context, treePos.x, treePos.y, snowImage);
            treePos.x = 28;
        }
        Gfx::drawImage(&context, treePos.x, treePos.y, image);
    }

    // 0x00500775
    constexpr std::array<uint8_t, 11> treeGrowth = { {
        1,
        0,
        1,
        2,
        2,
        3,
        4,
        5,
        6,
        0,
        0,
    } };

    uint8_t TreeObject::getTreeGrowthDisplayOffset() const
    {
        return treeGrowth[growth];
    }
}
