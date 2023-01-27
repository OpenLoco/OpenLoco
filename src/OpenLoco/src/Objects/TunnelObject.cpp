#include "TunnelObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <cassert>

namespace OpenLoco
{
    // 0x00469806
    void TunnelObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 16, y + 15, image);
        drawingCtx.drawImage(&rt, x - 16, y + 15, image + 1);
    }

    // 0x004697C9
    void TunnelObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(TunnelObject));

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

    // 0x004697EF
    void TunnelObject::unload()
    {
        name = 0;
        image = 0;
    }
}
