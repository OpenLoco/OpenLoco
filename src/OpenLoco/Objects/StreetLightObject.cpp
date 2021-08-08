#include "StreetLightObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // TODO: This should only be definined in the ObjectSelectionWindow header
    static const uint8_t descriptionRowHeight = 10;

    // 0x00477F69
    void StreetLightObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        Ui::Point imgPosition = Ui::Point{ x, y } - Ui::Point{ 20, 1 };
        for (auto i = 0; i < 3; i++)
        {
            auto imageId = (i * 4) + image;
            Gfx::drawImage(&context, imgPosition.x - 14, imgPosition.y, imageId + 2);
            Gfx::drawImage(&context, imgPosition.x, imgPosition.y - 7, imageId);
            imgPosition.x += 20;
            imgPosition.y += descriptionRowHeight;
        }
    }
}
