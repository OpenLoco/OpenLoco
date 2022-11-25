#include "ClimateObject.h"
#include "Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x00496B1B
    bool ClimateObject::validate() const
    {
        if (winterSnowLine > summerSnowLine)
        {
            return false;
        }
        return firstSeason < 4;
    }

    // 0x00496AF7
    void ClimateObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00496AF7, regs);
    }

    // 0x00496B15
    void ClimateObject::unload()
    {
        name = 0;
    }
}
