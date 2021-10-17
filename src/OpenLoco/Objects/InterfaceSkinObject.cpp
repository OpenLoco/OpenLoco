#include "InterfaceSkinObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x0043C86A
    void InterfaceSkinObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto image = Gfx::recolour(img + InterfaceSkin::ImageIds::preview_image, Colour::mutedSeaGreen);

        Gfx::drawImage(&context, x - 32, y - 32, image);
    }
}
