#include "DockObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00490F14
    void DockObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&rt, x - 34, y - 34, colourImage);
    }

    // 0x00490F2C
    void DockObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x00490EED
    bool DockObject::validate() const
    {
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

    // 0x00490E49
    void DockObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00490E49, regs);
    }

    // 0x00490EC9
    void DockObject::unload()
    {
        name = 0;
        image = 0;
        var_0C = 0;
        std::fill(std::begin(var_1C), std::end(var_1C), 0);
    }
}
