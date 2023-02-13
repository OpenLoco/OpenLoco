#include "RegionObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x0043CB93
    void RegionObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x, y, image);
    }

    // 0x0043CA8C
    void RegionObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(RegionObject));

        // Load object name string
        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load cargo? maybe default production?
        for (auto i = 0U; i < var_08; ++i)
        {
            ObjectHeader cargoHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(cargoHeader);
            }
            auto res = ObjectManager::findObjectHandle(cargoHeader);
            if (res.has_value())
            {
                var_09[i] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Find all the objects that this object loads
        while (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            ObjectHeader willLoadHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->willLoad.push_back(willLoadHeader);
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }
        remainingData = remainingData.subspan(sizeof(std::byte));

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        image = imageRes.imageOffset;

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);
    }

    // 0x0043CB6F
    void RegionObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(var_09), std::end(var_09), 0);
    }
}
