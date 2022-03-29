#include "SoundObject.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x0048AFAF
    void SoundObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> objData)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x0048AFAF, regs);
    }

    // 0x0048AFE1
    void SoundObject::unload()
    {
        name = 0;
        data = nullptr;
    }
}
