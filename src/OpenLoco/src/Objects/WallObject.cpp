#include "WallObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"

namespace OpenLoco
{
    // 0x004C4ACA
    void WallObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(WallObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        sprite = imageRes.imageOffset;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x004CAF0
    void WallObject::unload()
    {
        name = 0;
        sprite = 0;
    }

    // 0x004C4B0B
    void WallObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto image = sprite;
        if (flags & (1 << 6))
        {
            image = Gfx::recolour2(sprite, Colour::mutedDarkRed, Colour::yellow);
        }
        else
        {
            image = Gfx::recolour(sprite, Colour::mutedDarkRed);
        }

        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x + 14, y + 16 + (var_08 * 2), image);
        if (flags & (1 << 1))
        {
            drawingCtx.drawImage(&rt, x + 14, y + 16 + (var_08 * 2), Gfx::recolourTranslucent(sprite + 6, ExtColour::unk8C));
        }
        else
        {
            if (flags & (1 << 4))
            {
                drawingCtx.drawImage(&rt, x + 14, y + 16 + (var_08 * 2), image + 1);
            }
        }
    }
}
