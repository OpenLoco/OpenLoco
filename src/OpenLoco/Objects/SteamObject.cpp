#include "SteamObject.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00440CAD
    void SteamObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00440CAD, regs);
    }

    // 0x00440D8B
    void SteamObject::unload()
    {
        name = 0;
        baseImageId = 0;
        var_12 = 0;
        var_14 = 0;
        var_16 = nullptr;
        var_1A = nullptr;
        var_05 = 0;
        var_06 = 0;
        var_07 = 0;

        // Unsure of var_1F size 9th position might be a terminator
        std::fill_n(std::begin(var_1F), 8, 0);
    }
}
