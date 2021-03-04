#include "VehicleManager.h"
#include "../Company.h"
#include "../Interop/Interop.hpp"
#include "../Ptr.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::VehicleManager
{
    // 0x004C3A0C
    void determineAvailableVehicles(company& company)
    {
        registers regs;
        regs.esi = ToInt(&company);
        call(0x004C3A0C, regs);
    }
}
