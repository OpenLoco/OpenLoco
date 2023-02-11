#include "DockObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00490F14
    void DockObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 34, y - 34, colourImage);
    }

    // 0x00490F2C
    void DockObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x00490EED
    bool DockObject::validate() const
    {
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

    // 0x00490E49
    void DockObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(DockObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Load unk?
        var_14 = reinterpret_cast<const uint8_t*>(remainingData.data());
        remainingData = remainingData.subspan(numAux01);

        // Load unk2?
        var_18 = reinterpret_cast<const uint16_t*>(remainingData.data());
        remainingData = remainingData.subspan(numAux02Ent * sizeof(uint16_t));
        for (auto i = 0U; i < numAux02Ent; ++i)
        {
            var_1C[0] = reinterpret_cast<const uint8_t*>(remainingData.data());
            while (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                remainingData = remainingData.subspan(1);
            }
            remainingData = remainingData.subspan(1);
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;

        // Related to unk2?
        const auto offset = (flags & DockObjectFlags::unk01) != DockObjectFlags::none ? numAux02Ent * 4 : 1;
        var_0C = imgRes.imageOffset + offset;

        // Unused code numAux01 related

        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x00490EC9
    void DockObject::unload()
    {
        name = 0;
        image = 0;
        var_0C = 0;
        var_14 = nullptr;
        var_18 = nullptr;
        std::fill(std::begin(var_1C), std::end(var_1C), nullptr);
    }
}
