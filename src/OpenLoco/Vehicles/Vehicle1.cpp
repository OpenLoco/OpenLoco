#include "../Interop/Interop.hpp"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x004A9788
    bool Vehicle1::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A9788, regs) & X86_FLAG_CARRY);
    }
}
