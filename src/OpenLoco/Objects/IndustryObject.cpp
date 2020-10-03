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
    bool industry_object::requiresCargo() const
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

    bool industry_object::producesCargo() const
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

    char* industry_object::getProducedCargoString(const char* buffer)
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

                auto cargoObj = ObjectManager::get<cargo_object>(producedCargo);
                ptr = StringManager::formatString(ptr, cargoObj->name);
            }
        }
        return ptr;
    }

    char* industry_object::getRequiredCargoString(const char* buffer)
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

                auto cargoObj = ObjectManager::get<cargo_object>(requiredCargo);
                ptr = StringManager::formatString(ptr, cargoObj->name);
            }
        }
        return ptr;
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static const xy32 objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };

    // 0x0045932D
    void industry_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;

        xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        if (Gfx::clipDrawpixelinfo(&clipped, &dpi, pos, objectPreviewSize))
        {
            drawIndustry(clipped, objectPreviewOffset.x, 96);
        }
    }

    // 0x00458C7F
    void industry_object::drawIndustry(Gfx::drawpixelinfo_t* clipped, int16_t x, int16_t y) const
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.edi = (uint32_t)clipped;
        regs.ebp = (uint32_t)this;
        call(0x00458C7F, regs);
    }
}
