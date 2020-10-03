#include "InterfaceSkinObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // 0x0043C86A
    void interface_skin_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto image = Gfx::recolour(img + InterfaceSkin::ImageIds::preview_image, Colour::saturated_green);

        Gfx::drawImage(&dpi, x - 32, y - 32, image);
    }
}
