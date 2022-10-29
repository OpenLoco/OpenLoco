#include "IndustryObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Utility/Numeric.hpp"
#include "CargoObject.h"
#include "ObjectManager.h"
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
                    if ((flags & IndustryObjectFlags::requiresAllCargo) != 0)
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
        auto firstColour = Utility::bitScanReverse(var_C2);
        Colour c = firstColour != -1 ? static_cast<Colour>(firstColour)
                                     : Colour::black;
        ImageId baseImage(var_12, c);
        Ui::Point pos{ x, y };
        for (const auto part : getBuildingParts(0))
        {
            auto image = baseImage.withIndexOffset(part * 4 + 1);
            Gfx::drawImage(*clipped, pos, image);
            pos.y -= buildingPartHeight[part];
        }
    }

    // 0x0045926F
    bool IndustryObject::validate() const
    {
        if (var_1E == 0)
        {
            return false;
        }
        if (var_1F == 0 || var_1F > 31)
        {
            return false;
        }

        if (var_BD < var_BC)
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

        if (var_D6 > 100)
        {
            return false;
        }
        return var_DA <= 100;
    }

    // 0x00458CD9
    void IndustryObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00458CD9, regs);
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
        buildingPartHeight = nullptr;
        buildingPartAnimations = nullptr;
        std::fill(std::begin(animationSequences), std::end(animationSequences), nullptr);
        var_38 = 0;
        std::fill(std::begin(buildingParts), std::end(buildingParts), nullptr);
        var_BE = 0;
        std::fill(std::begin(producedCargoType), std::end(producedCargoType), 0);
        std::fill(std::begin(requiredCargoType), std::end(requiredCargoType), 0);
        std::fill(std::begin(wallTypes), std::end(wallTypes), 0);
        var_F1 = 0;
        var_F2 = 0;
    }

    stdx::span<const std::uint8_t> IndustryObject::getBuildingParts(const uint8_t buildingType) const
    {
        const auto* partsPointer = buildingParts[buildingType];
        auto* end = partsPointer;
        while (*end != 0xFF)
            end++;

        return stdx::span<const std::uint8_t>(partsPointer, end);
    }
    stdx::span<const std::uint8_t> IndustryObject::getAnimationSequence(const uint8_t unk) const
    {
        // animationSequences comprises of a size then then data. Size will always be a power of 2
        const auto* sequencePointer = animationSequences[unk];
        const auto size = *sequencePointer++;
        return stdx::span<const std::uint8_t>(sequencePointer, size);
    }
}
