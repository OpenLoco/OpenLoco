#include "HillShapesObject.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00463BBD
    void HillShapesObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto imageId = image + hillHeightMapCount + mountainHeightMapCount;

        Gfx::drawImage(&rt, x, y, imageId);
    }

    // 0x00463B70
    void HillShapesObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00463B70, regs);
    }

    // 0x00463B9F
    void HillShapesObject::unload()
    {
        name = 0;
        image = 0;
        var_08 = 0;
    }
}
