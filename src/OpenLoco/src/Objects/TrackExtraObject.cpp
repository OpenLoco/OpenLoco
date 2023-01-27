#include "TrackExtraObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <cassert>

namespace OpenLoco
{
    // 0x004A6D5F
    void TrackExtraObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (paintStyle == 0)
        {
            drawingCtx.drawImage(&rt, x, y, colourImage);
        }
        else
        {
            drawingCtx.drawImage(&rt, x, y, colourImage);
            drawingCtx.drawImage(&rt, x, y, colourImage + 97);
            drawingCtx.drawImage(&rt, x, y, colourImage + 96);
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
        auto remainingData = data.subspan(sizeof(TrackExtraObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        var_0E = imageRes.imageOffset;
        image = imageRes.imageOffset + 8;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x004A6D24
    void TrackExtraObject::unload()
    {
        name = 0;
        var_0E = 0;
        image = 0;
    }
}
