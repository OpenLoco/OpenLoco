#include "WallObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x004C4ACA
    void WallObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004C4ACA, regs);
    }

    // 0x004CAF0
    void WallObject::unload()
    {
        name = 0;
        sprite = 0;
    }

    // 0x004C4B0B
    void WallObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto image = sprite;
        if (flags & (1 << 6))
        {
            image = Gfx::recolour2(sprite, Colour::mutedDarkRed, Colour::yellow);
        }
        else
        {
            image = Gfx::recolour(sprite, Colour::mutedDarkRed);
        }

        Gfx::drawImage(&context, x + 14, y + 16 + (var_08 * 2), image);
        if (flags & (1 << 1))
        {
            Gfx::drawImage(&context, x + 14, y + 16 + (var_08 * 2), Gfx::recolourTranslucent(sprite + 6, ExtColour::unk8C));
        }
        else
        {
            if (flags & (1 << 4))
            {
                Gfx::drawImage(&context, x + 14, y + 16 + (var_08 * 2), image + 1);
            }
        }
    }
}
