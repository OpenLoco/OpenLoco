#include "SteamObject.h"
#include "Interop/Interop.hpp"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00440CAD
    void SteamObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00440CAD, regs);
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

    // 0x00440D8B
    void SteamObject::unload()
    {
        name = 0;
        baseImageId = 0;
        totalNumFramesType0 = 0;
        totalNumFramesType1 = 0;
        frameInfoType0 = nullptr;
        frameInfoType1 = nullptr;
        var_05 = 0;
        var_06 = 0;
        var_07 = 0;

        // Unsure of var_1F size 9th position might be a terminator
        std::fill_n(std::begin(var_1F), 8, 0);
    }
}
