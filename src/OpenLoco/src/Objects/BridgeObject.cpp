#include "BridgeObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0042C6A8
    void BridgeObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&rt, x - 21, y - 9, colourImage);
    }

    // 0x0042C651
    bool BridgeObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }

        if (-sellCostFactor > baseCostFactor)
        {
            return false;
        }
        if (baseCostFactor <= 0)
        {
            return false;
        }
        if (heightCostFactor < 0)
        {
            return false;
        }
        if (var_06 != 16 && var_06 != 32)
        {
            return false;
        }
        if (spanLength != 1 && spanLength != 2 && spanLength != 4)
        {
            return false;
        }
        if (trackNumCompatible > 7)
        {
            return false;
        }
        return roadNumCompatible <= 7;
    }

    // 0x0042C5B6
    void BridgeObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0042C5B6, regs);
    }

    // 0x0042C632
    void BridgeObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(trackMods), std::end(trackMods), 0);
        std::fill(std::begin(roadMods), std::end(roadMods), 0);
    }
}
