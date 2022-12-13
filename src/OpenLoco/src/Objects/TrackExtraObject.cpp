#include "TrackExtraObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x004A6D5F
    void TrackExtraObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        if (paintStyle == 0)
        {
            Gfx::drawImage(&rt, x, y, colourImage);
        }
        else
        {
            Gfx::drawImage(&rt, x, y, colourImage);
            Gfx::drawImage(&rt, x, y, colourImage + 97);
            Gfx::drawImage(&rt, x, y, colourImage + 96);
        }
    }

    // 0x004A6D38
    bool TrackExtraObject::validate() const
    {
        if (paintStyle >= 2)
        {
            return false;
        }

        // This check missing from vanilla
        if (costIndex > 32)
        {
            return false;
        }

        if (-sellCostFactor > buildCostFactor)
        {
            return false;
        }
        return buildCostFactor > 0;
    }

    // 0x004A6CF8
    void TrackExtraObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004A6CF8, regs);
    }

    // 0x004A6D24
    void TrackExtraObject::unload()
    {
        name = 0;
        var_0E = 0;
        image = 0;
    }
}
