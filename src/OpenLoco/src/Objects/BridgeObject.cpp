#include "BridgeObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"

namespace OpenLoco
{
    // 0x0042C6A8
    void BridgeObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 21, y - 9, colourImage);
    }

    // 0x0042C651
    bool BridgeObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }

        if (-sellCostFactor > baseCostFactor)
        {
            return false;
        }
        if (baseCostFactor <= 0)
        {
            return false;
        }
        if (heightCostFactor < 0)
        {
            return false;
        }
        if (var_06 != 16 && var_06 != 32)
        {
            return false;
        }
        if (spanLength != 1 && spanLength != 2 && spanLength != 4)
        {
            return false;
        }
        if (trackNumCompatible > 7)
        {
            return false;
        }
        return roadNumCompatible <= 7;
    }

    // 0x0042C5B6
    void BridgeObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(BridgeObject));

        {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
            name = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        }

        for (auto i = 0; i < trackNumCompatible; ++i)
        {
            trackMods[i] = 0xFF;
            ObjectHeader trackModHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandle(trackModHeader);
            if (res.has_value())
            {
                trackMods[i] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        for (auto i = 0; i < roadNumCompatible; ++i)
        {
            roadMods[i] = 0xFF;
            ObjectHeader roadModHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandle(roadModHeader);
            if (res.has_value())
            {
                roadMods[i] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x0042C632
    void BridgeObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(trackMods), std::end(trackMods), 0);
        std::fill(std::begin(roadMods), std::end(roadMods), 0);
    }
}
