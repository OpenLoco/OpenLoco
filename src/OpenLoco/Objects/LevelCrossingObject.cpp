#include "LevelCrossingObject.h"
#include "../Graphics/Gfx.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00478156
    void LevelCrossingObject::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
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

    // 0x004781A4
    void LevelCrossingObject::drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Gfx::point_t rowPosition = { x, y };
        ObjectManager::drawGenericDescription(dpi, rowPosition, designedYear, 0xFFFF);
    }
}
