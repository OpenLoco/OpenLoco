#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../GameException.hpp"
#include "../Graphics/Gfx.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../MultiPlayer.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../S5/S5.h"
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
    static loco_global<LoadOrQuitMode, 0x0050A002> _savePromptType;

    static loco_global<char[256], 0x0050B745> _currentScenarioFilename;
    static loco_global<char[512], 0x0112CE04> _savePath;

    // 0x004416FF
    static bool loadSaveGameOpen()
    {
        registers regs;
        call(0x004416FF, regs);
        return regs.eax;
    }

    // 0x00441843
    static bool saveSaveGameOpen()
    {
        registers regs;
        call(0x00441843, regs);
        return regs.eax;
    }

    // 0x004417A7
    static bool loadLandscapeOpen()
    {
        registers regs;
        call(0x004417A7, regs);
        return regs.eax;
    }

    // 0x00441993
    static bool saveLandscapeOpen()
    {
        registers regs;
        call(0x00441993, regs);
        return regs.eax;
    }

    // 0x00441FA7
    static bool sub_441FA7()
    {
        registers regs;
        call(0x00441FA7);
        return regs.eax != 0;
    }

    // 0x004424CE
    static bool sub_4424CE()
    {
        registers regs;
        call(0x004424CE);
        return regs.eax != 0;
    }

    // 0x0043BFF8
    static uint32_t loadGame()
    {
        GameCommands::do_21(1, 0);
        Input::toolCancel();

        if (isEditorMode())
        {
            if (loadLandscapeOpen())
            {
                // 0x0043C087
                auto path = fs::path(&_savePath[0]).replace_extension(S5::extensionSC5).u8string();
                std::strncpy(&_currentScenarioFilename[0], path.c_str(), std::size(_currentScenarioFilename));

                if (sub_4424CE())
                {
                    resetScreenAge();
                    throw GameException::Interrupt;
                }
            }
        }
        else if (!isNetworked())
        {
            if (loadSaveGameOpen())
            {
                // 0x0043C033
                auto path = fs::path(&_savePath[0]).replace_extension(S5::extensionSV5).u8string();
                std::strncpy(&_currentScenarioFilename[0], path.c_str(), std::size(_currentScenarioFilename));

                if (sub_441FA7())
                {
                    resetScreenAge();
                    throw GameException::Interrupt;
                }
            }
        }
        else if (isNetworked())
        {
            // 0x0043C0DB
            if (CompanyManager::getControllingId() == CompanyManager::updatingCompanyId())
            {
                MultiPlayer::setFlag(MultiPlayer::flags::flag_4);
                MultiPlayer::setFlag(MultiPlayer::flags::flag_3);
            }
        }

        // 0x0043C0D1
        Gfx::invalidateScreen();
        return 0;
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
                CompanyManager::setControllingId(0);
                CompanyManager::setSecondaryPlayerId(CompanyId::null);

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
        Input::toolCancel();

        if (isEditorMode())
        {
            if (saveLandscapeOpen())
            {
                // 0x0043C4B3
                auto path = fs::path(&_savePath[0]).replace_extension(S5::extensionSC5).u8string();
                std::strncpy(&_currentScenarioFilename[0], path.c_str(), std::size(_currentScenarioFilename));

                if (!S5::save(path, S5::SaveFlags::scenario))
                    Ui::Windows::Error::open(StringIds::landscape_save_failed, StringIds::null);
                else
                    GameCommands::do_21(2, 0);
            }
        }
        else if (!isNetworked())
        {
            if (saveSaveGameOpen())
            {
                // 0x0043C446
                auto path = fs::path(&_savePath[0]).replace_extension(S5::extensionSV5).u8string();
                std::strncpy(&_currentScenarioFilename[0], path.c_str(), std::size(_currentScenarioFilename));

                S5::SaveFlags flags = {};
                if (Config::get().flags & Config::flags::export_objects_with_saves)
                    flags = S5::SaveFlags::packCustomObjects;

                if (!S5::save(path, flags))
                    Ui::Windows::Error::open(StringIds::error_game_save_failed, StringIds::null);
                else
                    GameCommands::do_21(2, 0);
            }
        }
        else
        {
            // 0x0043C511
            GameCommands::do_72();
            MultiPlayer::setFlag(MultiPlayer::flags::flag_2);

            switch (_savePromptType)
            {
                case LoadOrQuitMode::LoadGamePrompt:
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_13); // intend to load?
                    break;
                case LoadOrQuitMode::ReturnToTitlePrompt:
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_14); // intend to return to title?
                    break;
                case LoadOrQuitMode::QuitGamePrompt:
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_15); // intend to quit game?
                    break;
            }
        }

        // 0x0043C411
        Gfx::invalidateScreen();
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
            _savePromptType = static_cast<LoadOrQuitMode>(di);
            Ui::Windows::TextInput::cancel();
            Ui::Windows::PromptSaveWindow::open(di);

            if (!isTitleMode())
            {
                // 0x0043C369
                // NB: tutorial recording has been omitted.
                if (Tutorial::state() == Tutorial::tutorial_state::playing)
                {
                    Tutorial::stop();
                }
                else if (!isNetworked() || _savePromptType != LoadOrQuitMode::QuitGamePrompt)
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
            case LoadOrQuitMode::LoadGamePrompt:
                return loadGame();

            case LoadOrQuitMode::ReturnToTitlePrompt:
                returnToTitle();
                return 0;

            case LoadOrQuitMode::QuitGamePrompt:
                return quitGame();
        }

        return 0;
    }

    void loadSaveQuit(registers& regs)
    {
        loadSaveQuit(regs.bl, regs.dl, regs.di);
    }
}
