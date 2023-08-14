#include "VehicleCrashEffect.h"

#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x004406A0
    void VehicleCrashParticle::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004406A0, regs);
    }
}
