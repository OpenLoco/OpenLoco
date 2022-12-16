#include "BuildingObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectManager.h"
#include <OpenLoco/Utility/Numeric.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x0042DE40
    void BuildingObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto bit = Utility::bitScanReverse(colours);

        const auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

        drawBuilding(&rt, 1, x, y + 40, colour);
    }

    // 0x0042DB95
    void BuildingObject::drawBuilding(Gfx::RenderTarget* clipped, uint8_t buildingRotation, int16_t x, int16_t y, Colour colour) const
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = enumValue(colour);
        regs.eax = buildingRotation;
        regs.edi = X86Pointer(clipped);
        regs.ebp = X86Pointer(this);
        call(0x0042DB95, regs);
    }

    // 0x0042DE82
    void BuildingObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x0042DE1E
    bool BuildingObject::validate() const
    {
        if ((var_06 == 0) || (var_06 > 63))
        {
            return false;
        }
        return (numVariations != 0) && (numVariations <= 31);
    }

    // 0x0042DBE8
    void BuildingObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0042DBE8, regs);
    }

    // 0x0042DDC4
    void BuildingObject::unload()
    {
        name = 0;
        image = 0;
        varationHeights = nullptr;
        var_0C = 0;
        std::fill(std::begin(variationsArr10), std::end(variationsArr10), nullptr);
        std::fill(std::begin(producedCargoType), std::end(producedCargoType), 0);
        std::fill(std::begin(var_A4), std::end(var_A4), 0);
        std::fill(std::begin(var_AE), std::end(var_AE), 0);
    }
}
