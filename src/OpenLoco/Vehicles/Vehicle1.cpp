#include "../Interop/Interop.hpp"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    // 0x004A9788
    bool Vehicle1::update()
    {
        switch (mode)
        {
            case TransportMode::air:
            case TransportMode::water:
                return true;
            case TransportMode::road:
                return updateRoad();
            case TransportMode::rail:
                return updateRail();
            default:
                return false;
        }
    }

    // 0x004A9969
    bool Vehicle1::updateRoad()
    {

        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A9969, regs) & X86_FLAG_CARRY);
    }

    // 0x004A97A6
    bool Vehicle1::updateRail()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        return !(call(0x004A97A6, regs) & X86_FLAG_CARRY);
    }
}
