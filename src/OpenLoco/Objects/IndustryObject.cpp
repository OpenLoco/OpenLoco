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
        for (const auto& requiredCargo : required_cargo_type)
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
        for (const auto& producedCargo : produced_cargo_type)
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

        for (const auto& producedCargo : produced_cargo_type)
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

        for (const auto& requiredCargo : required_cargo_type)
        {
            if (requiredCargo != 0xFF)
            {
                requiredCargoCount++;

                if (requiredCargoCount > 1)
                {
                    if ((flags & IndustryObjectFlags::requires_all_cargo) != 0)
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
    void IndustryObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        drawIndustry(&context, x, y + 40);
    }

    // 0x00458C7F
    void IndustryObject::drawIndustry(Gfx::Context* clipped, int16_t x, int16_t y) const
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.edi = X86Pointer(clipped);
        regs.ebp = X86Pointer(this);
        call(0x00458C7F, regs);
    }
}
