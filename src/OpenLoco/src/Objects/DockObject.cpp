#include "DockObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"

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
        partHeightsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        remainingData = remainingData.subspan(numBuildingParts);

        buildingPartAnimationsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        remainingData = remainingData.subspan(numBuildingParts * sizeof(BuildingPartAnimation));

        // Load building variation parts
        for (auto i = 0U; i < numBuildingVariations; ++i)
        {
            buildingVariationPartsOffset[i] = static_cast<uint32_t>(remainingData.data() - data.data());
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
        partHeightsOffset = 0;
        buildingPartAnimationsOffset = 0;
        std::fill(std::begin(buildingVariationPartsOffset), std::end(buildingVariationPartsOffset), 0);
    }

    std::span<const std::uint8_t> DockObject::getBuildingParts(const uint8_t buildingType) const
    {
        const auto offset = buildingVariationPartsOffset[buildingType];

        const auto* partsPointer = reinterpret_cast<const std::uint8_t*>(this) + offset;
        const auto* end = partsPointer;
        while (*end != 0xFF)
        {
            end++;
        }

        return std::span<const std::uint8_t>(partsPointer, end);
    }

    std::span<const BuildingPartAnimation> DockObject::getBuildingPartAnimations() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        const auto* ptr = reinterpret_cast<const BuildingPartAnimation*>(base + buildingPartAnimationsOffset);
        return std::span<const BuildingPartAnimation>(ptr, numBuildingParts);
    }

    std::span<const uint8_t> DockObject::getBuildingPartHeights() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        return std::span<const std::uint8_t>(base + partHeightsOffset, numBuildingParts);
    }

}
