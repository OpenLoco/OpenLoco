#include "RoadObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00477DFE
    void RoadObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);
        if (paintStyle == 1)
        {
            Gfx::drawImage(&context, x, y, colourImage + 34);
            Gfx::drawImage(&context, x, y, colourImage + 36);
            Gfx::drawImage(&context, x, y, colourImage + 38);
        }
        else
        {
            Gfx::drawImage(&context, x, y, colourImage + 34);
        }
    }

    // 0x00477DBC
    bool RoadObject::validate() const
    {
        // check missing in vanilla
        if (cost_index >= 32)
        {
            return false;
        }
        if (-sell_cost_factor > build_cost_factor)
        {
            return false;
        }
        if (build_cost_factor <= 0)
        {
            return false;
        }
        if (tunnel_cost_factor <= 0)
        {
            return false;
        }
        if (num_bridges > 7)
        {
            return false;
        }
        if (num_mods > 2)
        {
            return false;
        }
        if (flags & Flags12::unk_03)
        {
            return num_mods == 0;
        }
        return true;
    }

    // 0x00477BCF
    void RoadObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00477BCF, regs);
    }

    // 0x00477D9E
    void RoadObject::unload()
    {
        name = 0;
        var_0B = 0;
        image = 0;
        std::fill(std::begin(bridges), std::end(bridges), 0);
    }
}
