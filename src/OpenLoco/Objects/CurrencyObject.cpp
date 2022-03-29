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

    // 0x0046DFA9
    bool CurrencyObject::validate() const
    {
        if (separator > 4)
        {
            return false;
        }
        if (factor > 3)
        {
            return false;
        }
        return true;
    }

    // 0x0046DF56
    void CurrencyObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0046DF56, regs);
    }

    // 0x0046DF90
    void CurrencyObject::unload()
    {
        name = 0;
        prefix_symbol = 0;
        suffix_symbol = 0;
        object_icon = 0;
    }

    // 0x0046DFC3
    void CurrencyObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto currencyIndex = object_icon + 3;

        auto defaultElement = Gfx::getG1Element(ImageIds::currency_symbol);
        auto backupElement = *defaultElement;
        auto currencyElement = Gfx::getG1Element(currencyIndex);

        *defaultElement = *currencyElement;

        auto defaultWidth = _characterWidths[Font::large + 131];
        _characterWidths[Font::large + 131] = currencyElement->width + 1;

        Gfx::drawStringCentred(context, x, y - 9, Colour::black, StringIds::object_currency_big_font);

        _characterWidths[Font::large + 131] = defaultWidth;
        *defaultElement = backupElement;
    }
}
