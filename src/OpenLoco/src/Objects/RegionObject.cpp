#include "RegionObject.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0043CB93
    void RegionObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&rt, x, y, image);
    }

    // 0x0043CA8C
    void RegionObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0043CA8C, regs);
    }

    // 0x0043CB6F
    void RegionObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(var_09), std::end(var_09), 0);
    }
}
