#include "AirportObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00490DCF
    void AirportObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);
        drawingCtx.drawImage(x - 34, y - 34, colourImage);
    }

    // 0x00490DE7
    void AirportObject::drawDescription(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(drawingCtx, rowPosition, designedYear, obsoleteYear);
    }

    // 0x00490DA8
    bool AirportObject::validate() const
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

    // 0x00490CAF
    void AirportObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(AirportObject));

        {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
            name = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        }

        buildingPartHeightsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        remainingData = remainingData.subspan(numSpriteSets * sizeof(uint8_t));

        buildingPartAnimationsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        remainingData = remainingData.subspan(numSpriteSets * sizeof(BuildingPartAnimation));

        for (auto i = 0; i < numTiles; ++i)
        {
            buildingVariationPartOffsets[i] = remainingData.data() - data.data();
            while (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                remainingData = remainingData.subspan(1);
            }
            remainingData = remainingData.subspan(1);
        }

        buildingPositionsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        while (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            remainingData = remainingData.subspan(sizeof(AirportBuilding));
        }
        remainingData = remainingData.subspan(1);

        movementNodes = reinterpret_cast<const MovementNode*>(remainingData.data());
        remainingData = remainingData.subspan(numMovementNodes * sizeof(MovementNode));
        movementEdges = reinterpret_cast<const MovementEdge*>(remainingData.data());
        remainingData = remainingData.subspan(numMovementEdges * sizeof(MovementEdge));

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);

        auto nextImage = image + 1;
        if (hasFlags(AirportObjectFlags::hasShadows))
        {
            nextImage += numTiles * 4;
        }
        buildingImage = nextImage;
    }

    // 0x00490D66
    void AirportObject::unload()
    {
        name = 0;
        image = 0;
        buildingImage = 0;

        std::fill(std::begin(buildingVariationPartOffsets), std::end(buildingVariationPartOffsets), 0);

        buildingPositionsOffset = 0;

        movementNodes = nullptr;
        movementEdges = nullptr;
    }

    std::pair<World::TilePos2, World::TilePos2> AirportObject::getAirportExtents(const World::TilePos2& centrePos, const uint8_t rotation) const
    {
        World::TilePos2 minPos(minX, minY);
        World::TilePos2 maxPos(maxX, maxY);

        minPos = Math::Vector::rotate(minPos, rotation);
        maxPos = Math::Vector::rotate(maxPos, rotation);

        minPos += centrePos;
        maxPos += centrePos;

        if (minPos.x > maxPos.x)
        {
            std::swap(minPos.x, maxPos.x);
        }

        if (minPos.y > maxPos.y)
        {
            std::swap(minPos.y, maxPos.y);
        }
        return std::make_pair(minPos, maxPos);
    }

    std::span<const AirportBuilding> AirportObject::getBuildingPositions() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        const auto* buildingPositions = reinterpret_cast<const AirportBuilding*>(base + buildingPositionsOffset);

        const auto* firstBuildingPtr = buildingPositions;
        auto* endBuildingPtr = firstBuildingPtr;

        while (endBuildingPtr->index != 0xFF)
        {
            endBuildingPtr++;
        }

        return std::span<const AirportBuilding>(firstBuildingPtr, endBuildingPtr);
    }

    std::span<const std::uint8_t> AirportObject::getBuildingParts(const uint8_t buildingType) const
    {
        const auto offset = buildingVariationPartOffsets[buildingType];

        const auto* partsPointer = reinterpret_cast<const std::uint8_t*>(this) + offset;
        auto* end = partsPointer;
        while (*end != 0xFF)
        {
            end++;
        }

        return std::span<const std::uint8_t>(partsPointer, end);
    }

    std::span<const uint8_t> AirportObject::getBuildingPartHeights() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        return std::span<const std::uint8_t>(base + buildingPartHeightsOffset, numSpriteSets);
    }

    std::span<const BuildingPartAnimation> AirportObject::getBuildingPartAnimations() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        const auto* ptr = reinterpret_cast<const BuildingPartAnimation*>(base + buildingPartAnimationsOffset);
        return std::span<const BuildingPartAnimation>(ptr, numSpriteSets);
    }

}
