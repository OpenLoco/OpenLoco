#include "LevelCrossingObject.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../ScenarioManager.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x0047811A
    bool LevelCrossingObject::validate() const
    {
        if (-sellCostFactor > costFactor)
        {
            return false;
        }
        if (costFactor <= 0)
        {
            return false;
        }

        switch (closingFrames)
        {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16:
            case 32:
                return true;
            default:
                return false;
        }
    }

    // 0x004780E7
    void LevelCrossingObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004780E7, regs);
    }

    // 0x0047810D
    void LevelCrossingObject::unload()
    {
        name = 0;
        image = 0;
    }

    // 0x00478156
    void LevelCrossingObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto imageId = (closedFrames + 1) * 8;
        auto frameCount = (closingFrames - 1);
        auto animationFrame = frameCount & (ScenarioManager::getScenarioTicks() >> animationSpeed);
        auto frameIndex = 8 * animationFrame;
        imageId += frameIndex;
        imageId += image;

        Gfx::drawImage(&context, x, y, imageId);
        Gfx::drawImage(&context, x, y, imageId + 1);
        Gfx::drawImage(&context, x, y, imageId + 2);
        Gfx::drawImage(&context, x, y, imageId + 3);
    }

    // 0x004781A4
    void LevelCrossingObject::drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(context, rowPosition, designedYear, 0xFFFF);
    }
}
