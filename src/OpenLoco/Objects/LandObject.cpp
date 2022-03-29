#include "LandObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00469973
    bool LandObject::validate() const
    {
        if (cost_index > 32)
        {
            return false;
        }
        if (cost_factor <= 0)
        {
            return false;
        }
        if (var_03 < 1)
        {
            return false;
        }
        if (var_03 > 8)
        {
            return false;
        }

        return (var_04 == 1 || var_04 == 2 || var_04 == 4);
    }

    // 0x0046983C
    void LandObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0046983C, regs);
    }

    // 0x00469949
    void LandObject::unload()
    {
        name = 0;
        image = 0;
        var_0E = 0;
        var_12 = 0;
        var_06 = 0;
        var_07 = 0;
        var_16 = 0;
    }

    // 0x004699A8
    void LandObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        uint32_t imageId = image + (var_03 - 1) * var_0E;
        Gfx::drawImage(&context, x, y, imageId);
    }
}
