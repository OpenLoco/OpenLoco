#include "CliffEdgeObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

namespace OpenLoco
{
    // 0x004699C9
    void CliffEdgeObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(CliffEdgeObject));

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

    // 0x004699EF
    void CliffEdgeObject::unload()
    {
        name = 0;
        image = 0;
    }

    // 0x00469A06
    void CliffEdgeObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 30, y, image);
        drawingCtx.drawImage(&rt, x - 30, y, image + 16);
    }
}
