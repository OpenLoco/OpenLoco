#include "WaterObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
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
    void WaterObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(WaterObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        image = imageRes.imageOffset;
        mapPixelImage = imageRes.imageOffset + 40;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x004C56A8
    void WaterObject::unload()
    {
        name = 0;
        image = 0;
        mapPixelImage = 0;
    }

    // 0x004C56D3
    void WaterObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto colourImage = ImageId(image).withIndexOffset(35).withBlend(ExtColour::water);
        drawingCtx.drawImage(Ui::Point{ x, y }, colourImage);
        drawingCtx.drawImage(Ui::Point{ x, y }, ImageId(image).withIndexOffset(30));
    }
}
