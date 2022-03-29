#include "RockObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x004699C9
    void RockObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004699C9, regs);
    }

    // 0x004699EF
    void RockObject::unload()
    {
        name = 0;
        image = 0;
    }

    // 0x00469A06
    void RockObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&context, x - 30, y, image);
        Gfx::drawImage(&context, x - 30, y, image + 16);
    }
}
