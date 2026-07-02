#include "GameCommands/General/SetGameSpeed.h"
#include "GameCommands/GameCommands.h"
#include "SceneManager.h"

namespace OpenLoco::GameCommands
{
    static uint32_t setGameSpeed(const GameSpeed speed)
    {
        if (speed > GameSpeed::MAX)
        {
            return kFailure;
        }

        SceneManager::setGameSpeed(speed);
        return 0;
    }

    void setGameSpeed(registers& regs, const uint8_t)
    {
        SetGameSpeedArgs args(regs);
        regs.ebx = GameCommands::setGameSpeed(args.newSpeed);
    }
}
