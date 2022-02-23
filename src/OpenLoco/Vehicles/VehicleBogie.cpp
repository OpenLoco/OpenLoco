#include "../Interop/Interop.hpp"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x004AA008
    bool VehicleBogie::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004AA008, regs) & X86_FLAG_CARRY);
    }
}
