#include "LandObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectManager.h"

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
    void LandObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0046983C, regs);
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

    // 0x00469949
    void LandObject::unload()
    {
        name = 0;
        image = 0;
        var_0E = 0;
        var_12 = 0;
        var_06 = 0;
        var_07 = 0;
        var_16 = 0;
    }

    // 0x004699A8
    void LandObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        uint32_t imageId = image + (var_03 - 1) * var_0E;
        Gfx::drawImage(&rt, x, y, imageId);
    }
}
