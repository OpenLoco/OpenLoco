#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct LoadSaveQuitGameArgs
    {
        enum class Options : uint8_t
        {
            save,
            closeSavePrompt,
            dontSave,
        };
        static constexpr auto command = GameCommand::loadSaveQuitGame;

        LoadSaveQuitGameArgs() = default;
        explicit LoadSaveQuitGameArgs(const registers& regs)
            : option1(static_cast<Options>(regs.dl))
            , option2(static_cast<LoadOrQuitMode>(regs.di))
        {
        }

        Options option1;
        LoadOrQuitMode option2;

        explicit operator registers() const
        {
            registers regs;
            regs.dl = enumValue(option1); // [ 0 = save, 1 = close save prompt, 2 = don't save ]
            regs.di = enumValue(option2); // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
            return regs;
        }
    };

    void loadSaveQuit(registers& regs);
}
