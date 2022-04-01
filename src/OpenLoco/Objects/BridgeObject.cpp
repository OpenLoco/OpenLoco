#include "BridgeObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0042C6A8
    void BridgeObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&context, x - 21, y - 9, colourImage);
    }

    // 0x0042C651
    bool BridgeObject::validate() const
    {
        if (cost_index > 32)
        {
            return false;
        }

        if (-sell_cost_factor > base_cost_factor)
        {
            return false;
        }
        if (base_cost_factor <= 0)
        {
            return false;
        }
        if (height_cost_factor < 0)
        {
            return false;
        }
        if (var_06 != 16 && var_06 != 32)
        {
            return false;
        }
        if (span_length != 1 && span_length != 2 && span_length != 4)
        {
            return false;
        }
        if (track_num_compatible > 7)
        {
            return false;
        }
        return road_num_compatible <= 7;
    }

    // 0x0042C5B6
    void BridgeObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0042F4D0, regs);
    }

    // 0x0042C632
    void BridgeObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(track_mods), std::end(track_mods), 0);
        std::fill(std::begin(road_mods), std::end(road_mods), 0);
    }
}
