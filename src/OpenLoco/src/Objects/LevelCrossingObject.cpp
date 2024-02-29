#include "LevelCrossingObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include "ScenarioManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

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
    void LevelCrossingObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(LevelCrossingObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        image = imageRes.imageOffset;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x0047810D
    void LevelCrossingObject::unload()
    {
        name = 0;
        image = 0;
    }

    // 0x00478156
    void LevelCrossingObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto imageId = (closedFrames + 1) * 8;
        auto frameCount = (closingFrames - 1);
        auto animationFrame = frameCount & (ScenarioManager::getScenarioTicks() >> animationSpeed);
        auto frameIndex = 8 * animationFrame;
        imageId += frameIndex;
        imageId += image;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x, y, imageId);
        drawingCtx.drawImage(&rt, x, y, imageId + 1);
        drawingCtx.drawImage(&rt, x, y, imageId + 2);
        drawingCtx.drawImage(&rt, x, y, imageId + 3);
    }

    // 0x004781A4
    void LevelCrossingObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, 0xFFFF);
    }
}
