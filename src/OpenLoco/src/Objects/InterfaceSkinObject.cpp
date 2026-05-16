#include "InterfaceSkinObject.h"
#include "Graphics/Colour.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectStringTable.h"

namespace OpenLoco
{
    // 0x0043C82D
    void InterfaceSkinObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(InterfaceSkinObject));
        auto stringRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = stringRes.str;
        remainingData = remainingData.subspan(stringRes.tableLength);
        const auto initImageCount = ObjectManager::getTotalNumImages();
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        img = imageRes.imageOffset;
        const auto numImagesLoaded = ObjectManager::getTotalNumImages() - initImageCount;
        // Pad out the image table with images from the intrinsic interface
        // this is because existing interface skins were created before we
        // added more.
        for (auto i = numImagesLoaded; i < InterfaceSkin::ImageIds::kNewImageEnd; ++i)
        {
            *Gfx::getG1Element(img + i) = *Gfx::getG1Element(Gfx::G1ExpectedCount::kDisc + Gfx::G1ExpectedCount::kTemporaryObjects + i);
        }
        ObjectManager::setTotalNumImages(ObjectManager::getTotalNumImages() + InterfaceSkin::ImageIds::kNewImageEnd - numImagesLoaded);
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
