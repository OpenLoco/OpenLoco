#include "ScaffoldingObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0042DF15
    void ScaffoldingObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::yellow);

        Gfx::drawImage(&rt, x, y + 23, colourImage + Scaffolding::ImageIds::type21x1SegmentPart0);
        Gfx::drawImage(&rt, x, y + 23, colourImage + Scaffolding::ImageIds::type21x1SegmentPart1);
        Gfx::drawImage(&rt, x, y + 23, colourImage + Scaffolding::ImageIds::type21x1SegmentRoofSE);
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
