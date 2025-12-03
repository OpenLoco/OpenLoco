#include "BuildingObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco
{
    // 0x0042DE40
    void BuildingObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto bit = Numerics::bitScanReverse(colours);

        const auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

        drawBuilding(drawingCtx, 1, x, y + 40, colour);
    }

    // 0x0042DB95
    void BuildingObject::drawBuilding(Gfx::DrawingContext& drawingCtx, uint8_t buildingRotation, int16_t x, int16_t y, Colour colour) const
    {
        ImageId baseImage(image, colour);
        Ui::Point pos{ x, y };
        const auto partHeights = getBuildingPartHeights();
        for (const auto part : getBuildingParts(0))
        {
            auto partImage = baseImage.withIndexOffset(part * 4 + buildingRotation);
            drawingCtx.drawImage(pos, partImage);
            pos.y -= partHeights[part];
        }
    }

    // 0x0042DE82
    void BuildingObject::drawDescription(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(drawingCtx, rowPosition, designedYear, obsoleteYear);
    }

    // 0x0042DE1E
    bool BuildingObject::validate() const
    {
        if ((numParts == 0) || (numParts > 63))
        {
            return false;
        }
        return (numVariations != 0) && (numVariations <= 31);
    }

    // 0x0042DBE8
    void BuildingObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(BuildingObject));

        {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
            name = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        }

        // LOAD BUILDING PARTS Start
        // Load Part Heights
        partHeightsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        remainingData = remainingData.subspan(numParts * sizeof(uint8_t));

        // Load Part Animations (probably)
        partAnimationsOffset = static_cast<uint32_t>(remainingData.data() - data.data());
        remainingData = remainingData.subspan(numParts * sizeof(BuildingPartAnimation));

        // Load Parts
        for (auto i = 0; i < numVariations; ++i)
        {
            variationPartsOffset[i] = remainingData.data() - data.data();
            while (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                remainingData = remainingData.subspan(1);
            }
            remainingData = remainingData.subspan(1);
        }
        // LOAD BUILDING PARTS End

        // Load Produced Cargo
        for (auto& cargo : producedCargoType)
        {
            cargo = 0xFF;
            if (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                ObjectHeader cargoHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
                if (dependencies != nullptr)
                {
                    dependencies->required.push_back(cargoHeader);
                }
                auto res = ObjectManager::findObjectHandle(cargoHeader);
                if (res.has_value())
                {
                    cargo = res->id;
                }
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load Required Cargo
        for (auto& cargo : requiredCargoType)
        {
            cargo = 0xFF;
            if (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                ObjectHeader cargoHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());

                if (dependencies != nullptr)
                {
                    dependencies->required.push_back(cargoHeader);
                }
                auto res = ObjectManager::findObjectHandle(cargoHeader);
                if (res.has_value())
                {
                    cargo = res->id;
                }
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load Elevator Sequences
        for (auto i = 0; i < numElevatorSequences; ++i)
        {
            elevatorHeightSequencesOffset[i] = static_cast<uint32_t>(remainingData.data() - data.data());
            const auto size = *reinterpret_cast<const uint16_t*>(remainingData.data());
            remainingData = remainingData.subspan(sizeof(uint16_t) + size);
        }

        // Load Image Offsets
        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x0042DDC4
    void BuildingObject::unload()
    {
        name = 0;
        image = 0;
        partHeightsOffset = 0;
        partAnimationsOffset = 0;
        std::fill(std::begin(variationPartsOffset), std::end(variationPartsOffset), 0);
        std::fill(std::begin(producedCargoType), std::end(producedCargoType), 0);
        std::fill(std::begin(requiredCargoType), std::end(requiredCargoType), 0);
        std::fill(std::begin(elevatorHeightSequencesOffset), std::end(elevatorHeightSequencesOffset), 0);
    }

    std::span<const uint8_t> BuildingObject::getBuildingPartHeights() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        return std::span<const std::uint8_t>(base + partHeightsOffset, numParts);
    }

    std::span<const std::uint8_t> BuildingObject::getBuildingParts(const uint8_t variation) const
    {
        const auto* partsPointer = reinterpret_cast<const std::uint8_t*>(this) + variationPartsOffset[variation];
        const auto* end = partsPointer;
        while (*end != 0xFF)
        {
            end++;
        }

        return std::span<const std::uint8_t>(partsPointer, end);
    }

    std::span<const std::uint8_t> BuildingObject::getElevatorHeightSequence(const uint8_t animIdx) const
    {
        // elevatorHeightSequences comprises of a size (16bit) then data (8 bit). Size will always be a power of 2
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        const auto offset = elevatorHeightSequencesOffset[animIdx];
        const auto size = *reinterpret_cast<const uint16_t*>(base + offset);
        const auto* sequencePointer = base + offset + sizeof(uint16_t);
        return std::span<const std::uint8_t>(sequencePointer, size);
    }

    std::span<const BuildingPartAnimation> BuildingObject::getBuildingPartAnimations() const
    {
        const auto* base = reinterpret_cast<const uint8_t*>(this);
        const auto* ptr = reinterpret_cast<const BuildingPartAnimation*>(base + partAnimationsOffset);
        return std::span<const BuildingPartAnimation>(ptr, numParts);
    }

}
