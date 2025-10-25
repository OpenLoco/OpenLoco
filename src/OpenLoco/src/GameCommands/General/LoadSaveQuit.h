#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct LoadSaveQuitGameArgs
    {
        enum class SaveMode : uint8_t
        {
            save,
            closeSavePrompt,
            dontSave,
        };
        static constexpr auto command = GameCommand::loadSaveQuitGame;

        LoadSaveQuitGameArgs() = default;
        explicit LoadSaveQuitGameArgs(const registers& regs)
            : loadQuitMode(static_cast<LoadOrQuitMode>(regs.di))
            , saveMode(static_cast<SaveMode>(regs.dl))
        {
        }

        LoadOrQuitMode loadQuitMode;
        SaveMode saveMode;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(loadQuitMode); // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
            regs.dl = enumValue(saveMode);     // [ 0 = save, 1 = close save prompt, 2 = don't save ]
            return regs;
        }
    };

    void loadSaveQuit(registers& regs);
}
