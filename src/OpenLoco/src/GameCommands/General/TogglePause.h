#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct PauseGameArgs
    {
        static constexpr auto command = GameCommand::pauseGame;

        PauseGameArgs() = default;
        explicit PauseGameArgs(const registers&)
        {
        }

        explicit operator registers() const
        {
            return registers();
        }
    };

    void togglePause(registers& regs);
}
