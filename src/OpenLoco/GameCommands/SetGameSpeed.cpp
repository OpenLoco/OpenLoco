#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static uint32_t setGameSpeed(const GameSpeed speed)
    {
        if (speed > GameSpeed::MAX)
        {
            return FAILURE;
        }

        OpenLoco::setGameSpeed(speed);
        return 0;
    }

    void setGameSpeed(registers& regs)
    {
        SetGameSpeedArgs args(regs);
        regs.ebx = GameCommands::setGameSpeed(args.newSpeed);
    }
}
