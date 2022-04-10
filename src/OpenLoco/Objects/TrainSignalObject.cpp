#include "TrainSignalObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../ScenarioManager.h"

namespace OpenLoco
{
    // 0x0048995F
    bool TrainSignalObject::validate() const
    {
        // animationSpeed must be 1 less than a power of 2 (its a mask)
        switch (animationSpeed)
        {
            case 0:
            case 1:
            case 3:
            case 7:
            case 15:
                break;
            default:
                return false;
        }

        switch (num_frames)
        {
            case 4:
            case 7:
            case 10:
                break;
            default:
                return false;
        }

        if (cost_index > 32)
        {
            return false;
        }

        if (-sell_cost_factor > cost_factor)
        {
            return false;
        }

        if (num_compatible > 7)
        {
            return false;
        }
        return true;
    }

    // 0x004898E4
    void TrainSignalObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004898E4, regs);
    }

    // 0x0048993F
    void TrainSignalObject::unload()
    {
        name = 0;
        var_0C = 0;
        image = 0;
        std::fill(std::begin(mods), std::end(mods), 0);
    }

    // 0x004899A7
    void TrainSignalObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto frames = signalFrames[(((num_frames + 2) / 3) - 2)];
        auto frameCount = std::size(frames) - 1;
        auto animationFrame = frameCount & (ScenarioManager::getScenarioTicks() >> animationSpeed);

        auto frameIndex = frames[animationFrame];
        frameIndex *= 8;
        auto colourImage = image + frameIndex;

        Gfx::drawImage(&context, x, y + 15, colourImage);
    }
}
