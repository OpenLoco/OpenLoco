#include "AirportObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00490DCF
    void AirportObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&rt, x - 34, y - 34, colourImage);
    }

    // 0x00490DE7
    void AirportObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designed_year, obsolete_year);
    }

    // 0x00490DA8
    bool AirportObject::validate() const
    {
        if (cost_index > 32)
        {
            return false;
        }

        if (-sell_cost_factor > build_cost_factor)
        {
            return false;
        }

        return build_cost_factor > 0;
    }

    // 0x00490CAF
    void AirportObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00490CAF, regs);
    }

    // 0x00490D66
    void AirportObject::unload()
    {
        name = 0;
        image = 0;
        var_0C = 0;

        std::fill(std::begin(var_1C), std::end(var_1C), 0);

        var_9C = 0;

        movementNodes = nullptr;
        movementEdges = nullptr;
    }
}
