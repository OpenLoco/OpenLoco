#include "WaterObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x004C56BC
    bool WaterObject::validate() const
    {
        if (cost_index > 32)
        {
            return false;
        }
        if (cost_factor <= 0)
        {
            return false;
        }
        return true;
    }

    // 0x004C567C
    void WaterObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004C567C, regs);
    }

    // 0x004C56A8
    void WaterObject::unload()
    {
        name = 0;
        image = 0;
        var_0A = 0;
    }

    // 0x004C56D3
    void WaterObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolourTranslucent(Gfx::recolour(image + 35), ExtColour::null);
        Gfx::drawImage(&context, x, y, colourImage);
        Gfx::drawImage(&context, x, y, image + 30);
    }
}
