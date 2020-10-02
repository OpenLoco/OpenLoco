#include "LevelCrossingObject.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco
{
    void level_crossing_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        auto imageId = (closedFrames + 1) * 8;
        auto frameCount = (closingFrames - 1);
        auto animationFrame = frameCount & (scenarioTicks() >> animationSpeed);
        auto frameIndex = 8 * animationFrame;
        imageId += frameIndex;
        imageId += image;

        Gfx::drawImage(&dpi, x, y, imageId);
        Gfx::drawImage(&dpi, x, y, imageId + 1);
        Gfx::drawImage(&dpi, x, y, imageId + 2);
        Gfx::drawImage(&dpi, x, y, imageId + 3);
    }
}
