#include "CompetitorObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static const Gfx::point_t objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };

    // 0x00434D5B
    void competitor_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::point_t pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        Gfx::drawRect(&dpi, pos.x, pos.y, objectPreviewSize.width, objectPreviewSize.height, Colour::inset(Colour::dark_brown));

        auto image = Gfx::recolour(images[0], Colour::inset(Colour::dark_brown));
        Gfx::drawImage(&dpi, x - 32, y - 32, image);
    }
}
