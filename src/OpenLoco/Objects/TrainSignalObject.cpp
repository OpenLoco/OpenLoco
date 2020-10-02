#include "TrainSignalObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void train_signal_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto frames = signalFrames[(((num_frames + 2) / 3) - 2)];
        auto frameCount = std::size(frames) - 1;
        auto animationFrame = frameCount & (scenarioTicks() >> animationSpeed);

        auto frameIndex = frames[animationFrame];
        frameIndex *= 8;
        auto colourImage = image + frameIndex;

        Gfx::drawImage(&dpi, x, y + 15, colourImage);
    }
}
