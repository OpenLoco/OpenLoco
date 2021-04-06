#include "VehicleManager.h"
#include "../Company.h"
#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::VehicleManager
{
    // 0x004C3A0C
    void determineAvailableVehicles(Company& company)
    {
        registers regs;
        regs.esi = reinterpret_cast<int32_t>(&company);
        call(0x004C3A0C, regs);
    }
}
