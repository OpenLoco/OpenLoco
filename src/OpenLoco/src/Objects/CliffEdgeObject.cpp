#include "CliffEdgeObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x004699C9
    void CliffEdgeObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004699C9, regs);
    }

    // 0x004699EF
    void CliffEdgeObject::unload()
    {
        name = 0;
        image = 0;
    }

    // 0x00469A06
    void CliffEdgeObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        Gfx::drawImage(&rt, x - 30, y, image);
        Gfx::drawImage(&rt, x - 30, y, image + 16);
    }
}
