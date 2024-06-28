#include "ScaffoldingObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

namespace OpenLoco
{
    // 0x0042DF15
    void ScaffoldingObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::yellow);

        drawingCtx.drawImage(x, y + 23, colourImage + Scaffolding::ImageIds::type21x1SegmentBack);
        drawingCtx.drawImage(x, y + 23, colourImage + Scaffolding::ImageIds::type21x1SegmentFront);
        drawingCtx.drawImage(x, y + 23, colourImage + Scaffolding::ImageIds::type21x1RoofSE);
    }

    // 0x0042DED8
    void ScaffoldingObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(ScaffoldingObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        image = imageRes.imageOffset;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x0042DEFE
    void ScaffoldingObject::unload()
    {
        name = 0;
        image = 0;
    }
}
