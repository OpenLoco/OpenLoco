#include "../Interop/Interop.hpp"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x004A9B0B
    bool Vehicle2::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A9B0B, regs) & X86_FLAG_CARRY);
    }
}
