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
            : saveMode(static_cast<SaveMode>(regs.dl))
            , loadQuitMode(static_cast<LoadOrQuitMode>(regs.di))
        {
        }

        SaveMode saveMode;
        LoadOrQuitMode loadQuitMode;

        explicit operator registers() const
        {
            registers regs;
            regs.dl = enumValue(saveMode);     // [ 0 = save, 1 = close save prompt, 2 = don't save ]
            regs.di = enumValue(loadQuitMode); // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
            return regs;
        }
    };

    void loadSaveQuit(registers& regs);
}
