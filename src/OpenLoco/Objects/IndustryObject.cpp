#include "IndustryObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
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
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.edi = X86Pointer(clipped);
        regs.ebp = X86Pointer(this);
        call(0x00458C7F, regs);
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

        if (var_CE == 0 || var_CE > 32)
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
        var_20 = 0;
        var_24 = 0;
        std::fill(std::begin(var_28), std::end(var_28), 0);
        var_38 = 0;
        std::fill(std::begin(var_3C), std::end(var_3C), 0);
        var_BE = 0;
        std::fill(std::begin(producedCargoType), std::end(producedCargoType), 0);
        std::fill(std::begin(requiredCargoType), std::end(requiredCargoType), 0);
        var_ED = 0;
        var_EE = 0;
        var_EF = 0;
        var_F0 = 0;
        var_F1 = 0;
        var_F2 = 0;
    }
}
