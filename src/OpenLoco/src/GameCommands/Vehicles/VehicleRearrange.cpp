#include "VehicleRearrange.h"

namespace OpenLoco::GameCommands
{
    currency32_t vehicleRearrange(const VehicleRearrangeArgs& args, uint8_t flags)
    {
        args.dest;
        flags;
        return FAILURE;
    }

    // 0x004AF1DF
    void vehicleRearrange(registers& regs)
    {
        regs.ebx = vehicleRearrange(VehicleRearrangeArgs(regs), regs.bl);
    }
}
