#include "RoadExtraObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00477EB9
    void RoadExtraObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&rt, x, y, colourImage + 36);
        Gfx::drawImage(&rt, x, y, colourImage + 37);
        Gfx::drawImage(&rt, x, y, colourImage);
        Gfx::drawImage(&rt, x, y, colourImage + 33);
        Gfx::drawImage(&rt, x, y, colourImage + 32);
    }

    // 0x00477E92
    bool RoadExtraObject::validate() const
    {
        if (paintStyle >= 2)
        {
            return false;
        }

        // This check missing from vanilla
        if (costIndex >= 32)
        {
            return false;
        }

        if (-sellCostFactor > buildCostFactor)
        {
            return false;
        }
        return buildCostFactor > 0;
    }

    // 0x00477E52
    void RoadExtraObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00477E52, regs);
    }

    // 0x00477E7E
    void RoadExtraObject::unload()
    {
        name = 0;
        var_0E = 0;
        image = 0;
    }
}
