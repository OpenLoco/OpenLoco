#include "TrainSignalObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include "ScenarioManager.h"
#include <OpenLoco/Interop/Interop.hpp>

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

        switch (numFrames)
        {
            case 4:
            case 7:
            case 10:
                break;
            default:
                return false;
        }

        if (costIndex > 32)
        {
            return false;
        }

        if (-sellCostFactor > costFactor)
        {
            return false;
        }

        if (numCompatible > 7)
        {
            return false;
        }
        return true;
    }

    // 0x004898E4
    void TrainSignalObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(TrainSignalObject));

        auto loadString = [&remainingData, &handle](StringId& dst, uint8_t num) {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, num);
            dst = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        };

        loadString(name, 0);
        loadString(description, 1);

        // NOTE: These aren't dependent objects as this can load without the
        // related object.
        for (auto i = 0; i < numCompatible; ++i)
        {
            auto& mod = mods[i];
            mod = 0xFF;

            ObjectHeader modHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandle(modHeader);
            if (res.has_value())
            {
                mod = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x0048993F
    void TrainSignalObject::unload()
    {
        name = 0;
        description = 0;
        image = 0;
        std::fill(std::begin(mods), std::end(mods), 0);
    }

    // 0x004899A7
    void TrainSignalObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto& frames = signalFrames[(((numFrames + 2) / 3) - 2)];
        auto frameCount = std::size(frames) - 1;
        auto animationFrame = frameCount & (ScenarioManager::getScenarioTicks() >> animationSpeed);

        auto frameIndex = frames[animationFrame];
        frameIndex *= 8;
        auto colourImage = image + frameIndex;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x, y + 15, colourImage);
    }
}
