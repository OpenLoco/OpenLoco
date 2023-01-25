#include "InterfaceSkinObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectStringTable.h"

namespace OpenLoco
{
    // 0x0043C82D
    void InterfaceSkinObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
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
    void InterfaceSkinObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto image = Gfx::recolour(img + InterfaceSkin::ImageIds::preview_image, Colour::mutedSeaGreen);
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 32, y - 32, image);
    }
}
