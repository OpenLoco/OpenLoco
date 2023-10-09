#include "CurrencyObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

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
    void CurrencyObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(CurrencyObject));

        auto loadString = [&remainingData, &handle](StringId& dst, uint8_t num) {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, num);
            dst = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        };

        // Load object strings
        loadString(name, 0);
        loadString(prefixSymbol, 1);
        loadString(suffixSymbol, 2);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        objectIcon = imageRes.imageOffset;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x0046DF90
    void CurrencyObject::unload()
    {
        name = 0;
        prefixSymbol = 0;
        suffixSymbol = 0;
        objectIcon = 0;
    }

    // 0x0046DFC3
    void CurrencyObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto currencyIndex = objectIcon + 3;

        auto defaultElement = Gfx::getG1Element(ImageIds::currency_symbol);
        auto backupElement = *defaultElement;
        auto currencyElement = Gfx::getG1Element(currencyIndex);

        *defaultElement = *currencyElement;

        auto defaultWidth = _characterWidths[Font::large + 131];
        _characterWidths[Font::large + 131] = currencyElement->width + 1;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringCentred(rt, x, y - 9, Colour::black, StringIds::object_currency_big_font);

        _characterWidths[Font::large + 131] = defaultWidth;
        *defaultElement = backupElement;
    }
}
