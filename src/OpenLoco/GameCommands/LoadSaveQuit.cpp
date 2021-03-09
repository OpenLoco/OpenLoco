#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../GameException.hpp"
#include "../Graphics/Gfx.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Title.h"
#include "../Tutorial.h"
#include "../Ui/WindowManager.h"
#include "../Ui/WindowType.h"
#include "GameCommands.h"

#pragma warning(disable : 4702)

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<uint8_t, 0x00508F08> _game_command_nest_level;
    static loco_global<uint16_t, 0x0050A002> _savePromptType;

    // 0x0043BFF8
    static uint32_t loadGame()
    {
        registers regs;
        call(0x0043BFF8);
        return regs.ebx;
    }

    // 0x0043C182
    static uint32_t quitGame()
    {
        _game_command_nest_level = 0;

        // Path for networked games; untested.
        if (isNetworked())
        {
            clearScreenFlag(ScreenFlags::networked);
            auto playerCompanyId = CompanyManager::getControllingId();
            auto previousUpdatingId = CompanyManager::updatingCompanyId();
            CompanyManager::updatingCompanyId(playerCompanyId);

            Ui::WindowManager::closeAllFloatingWindows();

            CompanyManager::updatingCompanyId(previousUpdatingId);
            setScreenFlag(ScreenFlags::networked);

            // If the other party is leaving the game, go back to the title screen.
            if (playerCompanyId != previousUpdatingId)
            {
                // 0x0043C1CD
                addr<0x00F25428, uint32_t>() = 0;
                clearScreenFlag(ScreenFlags::networked);
                clearScreenFlag(ScreenFlags::networkHost);
                addr<0x00508F0C, uint32_t>() = 0;
                addr<0x00525E3C, uint8_t>() = 0;
                addr<0x00525E3D, uint8_t>() = 0xFF;

                Gfx::invalidateScreen();
                ObjectManager::loadIndex();

                Ui::WindowManager::close(Ui::WindowType::options);
                Ui::WindowManager::close(Ui::WindowType::companyFaceSelection);
                Ui::WindowManager::close(Ui::WindowType::objectSelection);

                clearScreenFlag(ScreenFlags::editor);
                Audio::pauseSound();
                Audio::unpauseSound();

                if (Input::hasFlag(Input::input_flags::flag5))
                {
                    Input::sub_407231();
                    Input::resetFlag(Input::input_flags::flag5);
                }

                Title::start();

                Ui::Windows::Error::open(StringIds::error_the_other_player_has_exited_the_game, StringIds::null);

                throw GameException::Interrupt;
            }
        }

        // 0x0043C1C8
        exitCleanly();
        return 0;
    }

    // 0x0043C0FD
    void returnToTitle()
    {
        if (isNetworked())
        {
            Ui::WindowManager::closeAllFloatingWindows();
        }

        Ui::WindowManager::close(Ui::WindowType::options);
        Ui::WindowManager::close(Ui::WindowType::companyFaceSelection);
        Ui::WindowManager::close(Ui::WindowType::objectSelection);
        Ui::WindowManager::close(Ui::WindowType::saveGamePrompt);

        clearScreenFlag(ScreenFlags::editor);
        Audio::pauseSound();
        Audio::unpauseSound();

        if (Input::hasFlag(Input::input_flags::flag5))
        {
            Input::sub_407231();
            Input::resetFlag(Input::input_flags::flag5);
        }

        Title::start();

        throw GameException::Interrupt;
    }

    // 0x0043C427
    void confirmSaveGame()
    {
        // registers regs;
        call(0x0043C427);
    }

    // 0x0043BFCB
    static uint32_t loadSaveQuit(const uint8_t flags, uint8_t dl, uint8_t di)
    {
        if ((flags & GameCommandFlag::apply) == 0)
            return 0;

        if (dl == 1)
        {
            Ui::WindowManager::close(Ui::WindowType::saveGamePrompt);
            return 0;
        }

        if (dl == 0)
        {
            _savePromptType = di;
            Ui::Windows::TextInput::cancel();
            Ui::Windows::PromptSaveWindow::open(_savePromptType);

            if (!isTitleMode())
            {
                // 0x0043C369
                if (Tutorial::state() == Tutorial::tutorial_state::playing)
                {
                    Tutorial::stop();
                }
                else if (!isNetworked() || _savePromptType != 2)
                {
                    if (getScreenAge() >= 0xF00)
                    {
                        auto window = Ui::WindowManager::bringToFront(Ui::WindowType::saveGamePrompt);
                        Audio::playSound(Audio::sound_id::open_window, window->x + (window->width / 2));
                        return 0;
                    }
                }
            }
        }

        // 0x0043BFE3
        switch (_savePromptType)
        {
            case 0:
                return loadGame();

            case 1:
                returnToTitle();
                return 0;

            case 2:
                return quitGame();
        }

        return 0;
    }

    void loadSaveQuit(registers& regs)
    {
        loadSaveQuit(regs.bl, regs.dl, regs.di);
    }
}
