#include "WaterObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <cassert>

namespace OpenLoco
{
    // 0x004C56BC
    bool WaterObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }
        if (costFactor <= 0)
        {
            return false;
        }
        return true;
    }

    // 0x004C567C
    void WaterObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(WaterObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        image = imageRes.imageOffset;
        var_0A = imageRes.imageOffset + 40;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x004C56A8
    void WaterObject::unload()
    {
        name = 0;
        image = 0;
        var_0A = 0;
    }

    // 0x004C56D3
    void WaterObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolourTranslucent(Gfx::recolour(image + 35), ExtColour::null);
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x, y, colourImage);
        drawingCtx.drawImage(&rt, x, y, image + 30);
    }
}
