#include "DockObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00490F14
    void DockObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);
        drawingCtx.drawImage(x - 34, y - 34, colourImage);
    }

    // 0x00490F2C
    void DockObject::drawDescription(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(drawingCtx, rowPosition, designedYear, obsoleteYear);
    }

    // 0x00490EED
    bool DockObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }

        if (-sellCostFactor > buildCostFactor)
        {
            return false;
        }

        return buildCostFactor > 0;
    }

    // 0x00490E49
    void DockObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(DockObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load part heights
        partHeights = reinterpret_cast<const uint8_t*>(remainingData.data());
        remainingData = remainingData.subspan(numBuildingParts);

        buildingPartAnimations = reinterpret_cast<const BuildingPartAnimation*>(remainingData.data());
        remainingData = remainingData.subspan(numBuildingParts * sizeof(uint16_t));

        // Load building variation parts
        for (auto i = 0U; i < numBuildingVariations; ++i)
        {
            buildingVariationParts[i] = reinterpret_cast<const uint8_t*>(remainingData.data());
            while (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                remainingData = remainingData.subspan(1);
            }
            remainingData = remainingData.subspan(1);
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;

        const auto offset = hasFlags(DockObjectFlags::hasShadows) ? (numBuildingVariations * 4) + 1 : 1;
        buildingImage = imgRes.imageOffset + offset;

        // Unused code numBuildingParts related

        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x00490EC9
    void DockObject::unload()
    {
        name = 0;
        image = 0;
        buildingImage = 0;
        partHeights = nullptr;
        buildingPartAnimations = nullptr;
        std::fill(std::begin(buildingVariationParts), std::end(buildingVariationParts), nullptr);
    }

    std::span<const std::uint8_t> DockObject::getBuildingParts(const uint8_t buildingType) const
    {
        const auto* partsPointer = buildingVariationParts[buildingType];
        auto* end = partsPointer;
        while (*end != 0xFF)
            end++;

        return std::span<const std::uint8_t>(partsPointer, end);
    }
}
