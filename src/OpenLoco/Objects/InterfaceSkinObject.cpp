#include "InterfaceSkinObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0043C82D
    void InterfaceSkinObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0043C82D, regs);
    }

    // 0x0043C853
    void InterfaceSkinObject::unload()
    {
        name = 0;
        img = 0;
    }

    // 0x0043C86A
    void InterfaceSkinObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto image = Gfx::recolour(img + InterfaceSkin::ImageIds::preview_image, Colour::mutedSeaGreen);

        Gfx::drawImage(&context, x - 32, y - 32, image);
    }
}
