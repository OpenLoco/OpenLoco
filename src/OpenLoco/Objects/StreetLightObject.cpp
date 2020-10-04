#include "StreetLightObject.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/Types.h"

namespace OpenLoco
{
    // TODO: This should only be definined in the ObjectSelectionWindow header
    static const uint8_t descriptionRowHeight = 10;

    // 0x00477F69
    void street_light_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::point_t imgPosition = Gfx::point_t{ x, y } - Gfx::point_t{ 20, 1 };
        for (auto i = 0; i < 3; i++)
        {
            auto imageId = (i * 4) + image;
            Gfx::drawImage(&dpi, imgPosition.x - 14, imgPosition.y, imageId + 2);
            Gfx::drawImage(&dpi, imgPosition.x, imgPosition.y - 7, imageId);
            imgPosition.x += 20;
            imgPosition.y += descriptionRowHeight;
        }
    }
}
