#include "InterfaceSkinObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x0043C82D
    void InterfaceSkinObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(InterfaceSkinObject));
        auto stringRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = stringRes.str;
        remainingData = remainingData.subspan(stringRes.tableLength);
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        img = imageRes.imageOffset;
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x0043C853
    void InterfaceSkinObject::unload()
    {
        name = 0;
        img = 0;
    }

    // 0x0043C86A
    void InterfaceSkinObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto image = Gfx::recolour(img + InterfaceSkin::ImageIds::preview_image, Colour::mutedSeaGreen);
        drawingCtx.drawImage(x - 32, y - 32, image);
    }
}
