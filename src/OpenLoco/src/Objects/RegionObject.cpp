#include "RegionObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Gfx.h"
#include "ObjectManager.h"
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
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0043CA8C, regs);
        if (dependencies != nullptr)
        {
            auto* depObjs = Interop::addr<0x0050D158, uint8_t*>();
            dependencies->required.resize(*depObjs++);
            if (!dependencies->required.empty())
            {
                std::copy(reinterpret_cast<ObjectHeader*>(depObjs), reinterpret_cast<ObjectHeader*>(depObjs) + dependencies->required.size(), dependencies->required.data());
                depObjs += sizeof(ObjectHeader) * dependencies->required.size();
            }
            dependencies->willLoad.resize(*depObjs++);
            if (!dependencies->willLoad.empty())
            {
                std::copy(reinterpret_cast<ObjectHeader*>(depObjs), reinterpret_cast<ObjectHeader*>(depObjs) + dependencies->willLoad.size(), dependencies->willLoad.data());
            }
        }
    }

    // 0x0043CB6F
    void RegionObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(var_09), std::end(var_09), 0);
    }
}
