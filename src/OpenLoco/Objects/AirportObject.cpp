#include "AirportObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00490DCF
    void AirportObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::salmon_pink);

        Gfx::drawImage(&context, x - 34, y - 34, colourImage);
    }

    // 0x00490DE7
    void AirportObject::drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(context, rowPosition, designed_year, obsolete_year);
    }
}
