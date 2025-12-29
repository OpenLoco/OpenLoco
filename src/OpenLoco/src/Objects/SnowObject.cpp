#include "SnowObject.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"

namespace OpenLoco
{
    // 0x00469A75
    void SnowObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        drawingCtx.drawImage(x, y, image);
    }

    // 0x00469A35
    void SnowObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(SnowObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset + SnowLine::ImageIds::surfaceFullZoom;
        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x00469A5E
    void SnowObject::unload()
    {
        name = 0;
        image = 0;
    }
}
