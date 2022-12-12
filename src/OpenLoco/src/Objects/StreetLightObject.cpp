#include "StreetLightObject.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // TODO: This should only be definined in the ObjectSelectionWindow header
    static constexpr uint8_t kDescriptionRowHeight = 10;

    // 0x00477F69
    void StreetLightObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        Ui::Point imgPosition = Ui::Point{ x, y } - Ui::Point{ 20, 1 };
        for (auto i = 0; i < 3; i++)
        {
            auto imageId = (i * 4) + image;
            Gfx::drawImage(&rt, imgPosition.x - 14, imgPosition.y, imageId + 2);
            Gfx::drawImage(&rt, imgPosition.x, imgPosition.y - 7, imageId);
            imgPosition.x += 20;
            imgPosition.y += kDescriptionRowHeight;
        }
    }

    // 0x00477F19
    void StreetLightObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00477F19, regs);
    }

    // 0x00477F52
    void StreetLightObject::unload()
    {
        name = 0;
        image = 0;
    }
}
