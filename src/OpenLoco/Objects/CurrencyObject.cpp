#include "CurrencyObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;

    // 0x0046DFC3
    void currency_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto currencyIndex = object_icon + 3;

        auto defaultElement = Gfx::getG1Element(ImageIds::currency_symbol);
        auto backupElement = *defaultElement;
        auto currencyElement = Gfx::getG1Element(currencyIndex);

        *defaultElement = *currencyElement;

        auto defaultWidth = _characterWidths[Font::large + 131];
        _characterWidths[Font::large + 131] = currencyElement->width + 1;

        Gfx::drawStringCentred(dpi, x, y - 9, Colour::black, StringIds::object_currency_big_font);

        _characterWidths[Font::large + 131] = defaultWidth;
        *defaultElement = backupElement;
    }
}
