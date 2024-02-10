#include "LandObject.h"
#include "CliffEdgeObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00469973
    bool LandObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }
        if (costFactor <= 0)
        {
            return false;
        }
        if (var_03 < 1)
        {
            return false;
        }
        if (var_03 > 8)
        {
            return false;
        }

        return (var_04 == 1 || var_04 == 2 || var_04 == 4);
    }

    // 0x0046983C
    void LandObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(LandObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        ObjectHeader cliffEdgeHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
        if (dependencies != nullptr)
        {
            dependencies->required.push_back(cliffEdgeHeader);
        }
        auto res = ObjectManager::findObjectHandle(cliffEdgeHeader);
        if (res.has_value())
        {
            cliffEdgeHeader1 = res->id;
            const auto* cliffObj = ObjectManager::get<CliffEdgeObject>(cliffEdgeHeader1);
            cliffEdgeImage = cliffObj->image;
        }
        remainingData = remainingData.subspan(sizeof(ObjectHeader));

        if (hasFlags(LandObjectFlags::unk1))
        {
            // TBC
            ObjectHeader cliffEdgeHeader2_ = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(cliffEdgeHeader2_);
            }
            auto res2 = ObjectManager::findObjectHandle(cliffEdgeHeader2_);
            if (res2.has_value())
            {
                cliffEdgeHeader2 = res2->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);

        var_0E = var_04 * 25;
        image = var_04 * var_03 * 57 + imgRes.imageOffset;
        mapPixelImage = var_04 * var_03 * 82 + imgRes.imageOffset;

        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x00469949
    void LandObject::unload()
    {
        name = 0;
        image = 0;
        var_0E = 0;
        cliffEdgeImage = 0;
        cliffEdgeHeader1 = 0;
        cliffEdgeHeader2 = 0;
        mapPixelImage = 0;
    }

    // 0x004699A8
    void LandObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        uint32_t imageId = image + (var_03 - 1) * var_0E;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x, y, imageId);
    }
}
