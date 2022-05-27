#include "ScenarioTextObject.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0043EDE3
    void ScenarioTextObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0043EDE3, regs);
    }

    // 0x0043EE0B
    void ScenarioTextObject::unload()
    {
        name = 0;
        details = 0;
    }
}
