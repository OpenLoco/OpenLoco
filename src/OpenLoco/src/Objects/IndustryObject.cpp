#include "IndustryObject.h"
#include "CargoObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    bool IndustryObject::requiresCargo() const
    {
        auto requiredCargoState = false;
        for (const auto& requiredCargo : requiredCargoType)
        {
            if (requiredCargo != 0xff)
            {
                requiredCargoState = true;
                break;
            }
        }
        return requiredCargoState;
    }

    bool IndustryObject::producesCargo() const
    {
        auto produceCargoState = false;
        for (const auto& producedCargo : producedCargoType)
        {
            if (producedCargo != 0xff)
            {
                produceCargoState = true;
                break;
            }
        }
        return produceCargoState;
    }

    char* IndustryObject::getProducedCargoString(const char* buffer) const
    {
        char* ptr = (char*)buffer;
        auto producedCargoCount = 0;

        for (const auto& producedCargo : producedCargoType)
        {
            if (producedCargo != 0xFF)
            {
                producedCargoCount++;

                if (producedCargoCount > 1)
                    ptr = StringManager::formatString(ptr, StringIds::cargo_and);

                auto cargoObj = ObjectManager::get<CargoObject>(producedCargo);
                ptr = StringManager::formatString(ptr, cargoObj->name);
            }
        }
        return ptr;
    }

    char* IndustryObject::getRequiredCargoString(const char* buffer) const
    {
        char* ptr = (char*)buffer;
        auto requiredCargoCount = 0;

        for (const auto& requiredCargo : requiredCargoType)
        {
            if (requiredCargo != 0xFF)
            {
                requiredCargoCount++;

                if (requiredCargoCount > 1)
                {
                    if (hasFlags(IndustryObjectFlags::requiresAllCargo))
                        ptr = StringManager::formatString(ptr, StringIds::cargo_and);
                    else
                        ptr = StringManager::formatString(ptr, StringIds::cargo_or);
                }

                auto cargoObj = ObjectManager::get<CargoObject>(requiredCargo);
                ptr = StringManager::formatString(ptr, cargoObj->name);
            }
        }
        return ptr;
    }

    // 0x0045932D
    void IndustryObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        drawIndustry(&rt, x, y + 40);
    }

    // 0x00458C7F
    void IndustryObject::drawIndustry(Gfx::RenderTarget* clipped, int16_t x, int16_t y) const
    {
        auto firstColour = Numerics::bitScanReverse(availableColours);
        Colour c = firstColour != -1 ? static_cast<Colour>(firstColour)
                                     : Colour::black;
        ImageId baseImage(var_12, c);
        Ui::Point pos{ x, y };
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        for (const auto part : getBuildingParts(0))
        {
            auto image = baseImage.withIndexOffset(part * 4 + 1);
            drawingCtx.drawImage(*clipped, pos, image);
            pos.y -= buildingPartHeights[part];
        }
    }

    // 0x0045926F
    bool IndustryObject::validate() const
    {
        if (numBuildingParts == 0)
        {
            return false;
        }
        if (numBuildingVariations == 0 || numBuildingVariations > 31)
        {
            return false;
        }

        if (maxNumBuildings < minNumBuildings)
        {
            return false;
        }

        if (totalOfTypeInScenario == 0 || totalOfTypeInScenario > 32)
        {
            return false;
        }

        // 230/256 = ~90%
        if (-clearCostFactor > costFactor * 230 / 256)
        {
            return false;
        }

        if (var_E8 > 8)
        {
            return false;
        }
        switch (var_E9)
        {
            case 1:
            case 2:
            case 4:
                break;
            default:
                return false;
        }

        if (var_EA != 0xFF && var_EA > 7)
        {
            return false;
        }

        if (var_EC > 8)
        {
            return false;
        }

        if (initialProductionRate[0].min > 100)
        {
            return false;
        }
        return initialProductionRate[1].min <= 100;
    }

    // 0x00458CD9
    void IndustryObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(IndustryObject));

        {
            auto loadString = [&remainingData, &handle](StringId& dst, uint8_t num) {
                auto strRes = ObjectManager::loadStringTable(remainingData, handle, num);
                dst = strRes.str;
                remainingData = remainingData.subspan(strRes.tableLength);
            };

            // This is actually used for initial name of the industry when two industries in same town
            StringId notUsed{};

            loadString(name, 0);
            loadString(var_02, 1);
            loadString(notUsed, 2);
            loadString(nameClosingDown, 3);
            loadString(nameUpProduction, 4);
            loadString(nameDownProduction, 5);
            loadString(nameSingular, 6);
            loadString(namePlural, 7);
        }

        // LOAD BUILDING VARIATION PARTS Start
        // Load variation heights
        buildingPartHeights = reinterpret_cast<const uint8_t*>(remainingData.data());
        remainingData = remainingData.subspan(numBuildingParts * sizeof(uint8_t));

        // Load Part Animations
        buildingPartAnimations = reinterpret_cast<const BuildingPartAnimation*>(remainingData.data());
        remainingData = remainingData.subspan(numBuildingParts * sizeof(BuildingPartAnimation));

        // Load Animations
        for (auto& animSeq : animationSequences)
        {
            animSeq = reinterpret_cast<const uint8_t*>(remainingData.data());
            // animationSequences comprises of a size then data. Size will always be a power of 2
            remainingData = remainingData.subspan(*animSeq * sizeof(uint8_t) + 1);
        }

        // Load Unk Animation Related Structure
        var_38 = reinterpret_cast<const IndustryObjectUnk38*>(remainingData.data());
        while (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            remainingData = remainingData.subspan(sizeof(IndustryObjectUnk38));
        }
        remainingData = remainingData.subspan(1);

        // Load Parts
        for (auto i = 0; i < numBuildingVariations; ++i)
        {
            auto& part = buildingVariationParts[i];
            part = reinterpret_cast<const uint8_t*>(remainingData.data());
            while (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                remainingData = remainingData.subspan(1);
            }
            remainingData = remainingData.subspan(1);
        }
        // LOAD BUILDING PARTS End

        // Load Unk?
        buildings = reinterpret_cast<const uint8_t*>(remainingData.data());
        remainingData = remainingData.subspan(maxNumBuildings * sizeof(uint8_t));

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

        // Load Wall Types
        for (auto& wallType : wallTypes)
        {
            wallType = 0xFF;
            if (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                ObjectHeader wallHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());

                if (dependencies != nullptr)
                {
                    dependencies->required.push_back(wallHeader);
                }
                auto res = ObjectManager::findObjectHandle(wallHeader);
                if (res.has_value())
                {
                    wallType = res->id;
                }
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load Unk1 Wall Types
        buildingWall = 0xFF;
        if (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            ObjectHeader unkHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(unkHeader);
            }
            auto res = ObjectManager::findObjectHandle(unkHeader);
            if (res.has_value())
            {
                buildingWall = res->id;
            }
        }
        remainingData = remainingData.subspan(sizeof(ObjectHeader));

        // Load Unk2 Wall Types
        buildingWallEntrance = 0xFF;
        if (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            ObjectHeader unkHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(unkHeader);
            }
            auto res = ObjectManager::findObjectHandle(unkHeader);
            if (res.has_value())
            {
                buildingWallEntrance = res->id;
            }
        }
        remainingData = remainingData.subspan(sizeof(ObjectHeader));

        // Load Image Offsets
        auto imgRes = ObjectManager::loadImageTable(remainingData);
        var_0E = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);
        var_12 = var_0E;
        if (hasFlags(IndustryObjectFlags::hasShadows))
        {
            var_12 += numBuildingVariations * 4;
        }
        var_16 = numBuildingParts * 4 + var_12;
        var_1A = var_E9 * 21;
    }

    // 0x0045919D
    void IndustryObject::unload()
    {
        name = 0;
        var_02 = 0;
        nameClosingDown = 0;
        nameUpProduction = 0;
        nameDownProduction = 0;
        nameSingular = 0;
        namePlural = 0;

        var_0E = 0;
        var_12 = 0;
        var_16 = 0;
        var_1A = 0;
        buildingPartHeights = nullptr;
        buildingPartAnimations = nullptr;
        std::fill(std::begin(animationSequences), std::end(animationSequences), nullptr);
        var_38 = nullptr;
        std::fill(std::begin(buildingVariationParts), std::end(buildingVariationParts), nullptr);
        buildings = nullptr;
        std::fill(std::begin(producedCargoType), std::end(producedCargoType), 0);
        std::fill(std::begin(requiredCargoType), std::end(requiredCargoType), 0);
        std::fill(std::begin(wallTypes), std::end(wallTypes), 0);
        buildingWall = 0;
        buildingWallEntrance = 0;
    }

    std::span<const std::uint8_t> IndustryObject::getBuildingParts(const uint8_t buildingType) const
    {
        const auto* partsPointer = buildingVariationParts[buildingType];
        auto* end = partsPointer;
        while (*end != 0xFF)
            end++;

        return std::span<const std::uint8_t>(partsPointer, end);
    }

    std::span<const std::uint8_t> IndustryObject::getAnimationSequence(const uint8_t unk) const
    {
        // animationSequences comprises of a size then data. Size will always be a power of 2
        const auto* sequencePointer = animationSequences[unk];
        const auto size = *sequencePointer++;
        return std::span<const std::uint8_t>(sequencePointer, size);
    }

    std::span<const IndustryObjectUnk38> OpenLoco::IndustryObject::getUnk38() const
    {
        const auto* unkPointer = var_38;
        auto* end = unkPointer;
        while (end->var_00 != 0xFF)
            end++;
        return std::span<const IndustryObjectUnk38>(unkPointer, end);
    }
}
