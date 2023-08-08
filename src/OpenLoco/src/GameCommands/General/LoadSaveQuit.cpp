#include "Audio/Audio.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "SceneManager.h"
#include "Tutorial.h"
#include "Ui/WindowManager.h"
#include "Ui/WindowType.h"

#pragma warning(disable : 4702)

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<LoadOrQuitMode, 0x0050A002> _savePromptType;

    // 0x0043BFCB
    static uint32_t loadSaveQuit(const LoadSaveQuitGameArgs& args, const uint8_t flags)
    {
        if ((flags & Flags::apply) == 0)
            return 0;

        if (args.option1 == LoadSaveQuitGameArgs::Options::closeSavePrompt)
        {
            Ui::WindowManager::close(Ui::WindowType::saveGamePrompt);
            return 0;
        }

        if (args.option1 == LoadSaveQuitGameArgs::Options::save)
        {
            _savePromptType = args.option2;
            Ui::Windows::TextInput::cancel();
            Ui::Windows::PromptSaveWindow::open(args.option2);

            if (!isTitleMode())
            {
                // 0x0043C369
                // NB: tutorial recording has been omitted.
                if (Tutorial::state() == Tutorial::State::playing)
                {
                    Tutorial::stop();
                }
                else if (!isNetworked() || _savePromptType != LoadOrQuitMode::quitGamePrompt)
                {
                    if (getScreenAge() >= 0xF00)
                    {
                        auto window = Ui::WindowManager::bringToFront(Ui::WindowType::saveGamePrompt);
                        Audio::playSound(Audio::SoundId::openWindow, window->x + (window->width / 2));
                        return 0;
                    }
                }
            }
        }

        // 0x0043BFE3
        switch (_savePromptType)
        {
            case LoadOrQuitMode::loadGamePrompt:
                Game::loadGame();
                break;

            case LoadOrQuitMode::returnToTitlePrompt:
                Game::returnToTitle();
                break;

            case LoadOrQuitMode::quitGamePrompt:
                Game::quitGame();
                break;
        }

        return 0;
    }

    void loadSaveQuit(registers& regs)
    {
        regs.ebx = loadSaveQuit(LoadSaveQuitGameArgs(regs), regs.bl);
    }
}
