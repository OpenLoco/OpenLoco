#include "ScaffoldingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0042DF15
    void ScaffoldingObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::yellow);

        Gfx::drawImage(&context, x, y + 23, colourImage + 24);
        Gfx::drawImage(&context, x, y + 23, colourImage + 25);
        Gfx::drawImage(&context, x, y + 23, colourImage + 27);
    }

    // 0x0042DED8
    void ScaffoldingObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0042DED8, regs);
    }

    // 0x0042DEFE
    void ScaffoldingObject::unload()
    {
        name = 0;
        image = 0;
    }
}
