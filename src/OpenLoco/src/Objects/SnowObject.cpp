#include "SnowObject.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00469A75
    void SnowObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&rt, x, y, image);
    }

    // 0x00469A35
    void SnowObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00469A35, regs);
    }

    // 0x00469A5E
    void SnowObject::unload()
    {
        name = 0;
        image = 0;
    }
}
