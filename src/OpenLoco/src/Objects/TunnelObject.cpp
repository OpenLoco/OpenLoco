#include "TunnelObject.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00469806
    void TunnelObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&rt, x - 16, y + 15, image);
        Gfx::drawImage(&rt, x - 16, y + 15, image + 1);
    }

    // 0x004697C9
    void TunnelObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004697C9, regs);
    }

    // 0x004697EF
    void TunnelObject::unload()
    {
        name = 0;
        image = 0;
    }
}
