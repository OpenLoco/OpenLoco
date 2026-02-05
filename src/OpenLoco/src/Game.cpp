#include "Game.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Environment.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "GameException.hpp"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "MultiPlayer.h"
#include "Objects/ObjectIndex.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "Scenario/Scenario.h"
#include "Scenario/ScenarioOptions.h"
#include "SceneManager.h"
#include "Title.h"
#include "Ui/ProgressBar.h"
#include "Ui/ToolManager.h"
#include "Ui/WindowManager.h"
#include "Ui/WindowType.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Game
{
    // TODO: move into GameState?
    static std::string _activeSavePath; // 0x0050B745

    using Ui::Windows::PromptBrowse::browse_type;

    static std::optional<std::string> openBrowsePrompt(std::string path, StringId titleId, browse_type type, const char* filter)
    {
        SceneManager::setPauseFlag(PauseFlags::browsePrompt);
        Gfx::invalidateScreen();
        Gfx::renderAndUpdate();

        auto confirm = Ui::Windows::PromptBrowse::open(type, path, filter, titleId);

        Input::processMessagesMini();
        SceneManager::unsetPauseFlag(PauseFlags::browsePrompt);
        Gfx::invalidateScreen();
        Gfx::renderAndUpdate();

        return confirm;
    }

    // 0x004416FF
    [[nodiscard]] std::optional<std::string> loadSaveGameOpen()
    {
        auto path = Environment::getPath(Environment::PathId::save).make_preferred().u8string();

        return openBrowsePrompt(path, StringIds::title_prompt_load_game, browse_type::load, S5::filterSV5);
    }

    // 0x004417A7
    [[nodiscard]] std::optional<std::string> loadLandscapeOpen()
    {
        auto path = Environment::getPath(Environment::PathId::landscape).make_preferred().u8string();

        return openBrowsePrompt(path, StringIds::title_prompt_load_landscape, browse_type::load, S5::filterSC5);
    }

    [[nodiscard]] std::optional<std::string> loadHeightmapOpen()
    {
        fs::path basePath = Environment::getPath(Environment::PathId::heightmap);
        Environment::autoCreateDirectory(basePath);
        auto path = basePath.make_preferred().u8string();

        // TODO: make named constant for filter?
        return openBrowsePrompt(path, StringIds::title_load_png_heightmap_file, browse_type::load, "*.png");
    }

    // 0x00441843
    [[nodiscard]] std::optional<std::string> saveSaveGameOpen()
    {
        auto path = _activeSavePath;

        return openBrowsePrompt(path, StringIds::title_prompt_save_game, browse_type::save, S5::filterSV5);
    }

    // 0x004418DB
    [[nodiscard]] std::optional<std::string> saveScenarioOpen()
    {
        auto path = Environment::getPath(Environment::PathId::scenarios) / Scenario::getOptions().scenarioName;
        auto savePath = path.u8string() + S5::extensionSC5;

        return openBrowsePrompt(savePath, StringIds::title_prompt_save_scenario, browse_type::save, S5::filterSC5);
    }

    // 0x00441993
    [[nodiscard]] std::optional<std::string> saveLandscapeOpen()
    {
        Scenario::getOptions().scenarioFlags &= ~Scenario::ScenarioFlags::landscapeGenerationDone;
        if (hasFlags(GameStateFlags::tileManagerLoaded))
        {
            Scenario::getOptions().scenarioFlags |= Scenario::ScenarioFlags::landscapeGenerationDone;
            Scenario::drawScenarioMiniMapImage();
        }

        auto path = Environment::getPath(Environment::PathId::landscape) / Scenario::getOptions().scenarioName;
        auto savePath = path.u8string() + S5::extensionSC5;

        return openBrowsePrompt(savePath, StringIds::title_prompt_save_landscape, browse_type::save, S5::filterSC5);
    }

    // 0x0043BFF8
    void loadGame()
    {
        GameCommands::LoadSaveQuitGameArgs args{};
        args.loadQuitMode = LoadOrQuitMode::loadGamePrompt;
        args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::closeSavePrompt;
        GameCommands::doCommand(args, GameCommands::Flags::apply);

        ToolManager::toolCancel();

        if (SceneManager::isEditorMode())
        {
            if (auto res = Game::loadLandscapeOpen())
            {
                // 0x0043C087
                auto path = fs::u8path(*res).replace_extension(S5::extensionSC5);
                _activeSavePath = path.u8string();

                // 0x004424CE
                if (S5::importSaveToGameState(path, S5::LoadFlags::landscape))
                {
                    SceneManager::resetSceneAge();
                    throw GameException::Interrupt;
                }
            }
        }
        else if (!SceneManager::isNetworked())
        {
            if (auto res = Game::loadSaveGameOpen())
            {
                // 0x0043C033
                auto path = fs::u8path(*res).replace_extension(S5::extensionSV5);
                _activeSavePath = path.u8string();

                if (S5::importSaveToGameState(path, S5::LoadFlags::none))
                {
                    SceneManager::resetSceneAge();
                    throw GameException::Interrupt;
                }
            }
        }
        else if (SceneManager::isNetworked())
        {
            // 0x0043C0DB
            if (CompanyManager::getControllingId() == GameCommands::getUpdatingCompanyId())
            {
                MultiPlayer::setFlag(MultiPlayer::flags::flag_4);
                MultiPlayer::setFlag(MultiPlayer::flags::flag_3);
            }
        }

        // 0x0043C0D1
        Gfx::invalidateScreen();
    }

    // 0x0043C182
    void quitGame()
    {
        GameCommands::resetCommandNestLevel();

        // Path for networked games; untested.
        if (SceneManager::isNetworked())
        {
            SceneManager::removeSceneFlags(SceneManager::Flags::networked);
            auto playerCompanyId = CompanyManager::getControllingId();
            auto previousUpdatingId = GameCommands::getUpdatingCompanyId();
            GameCommands::setUpdatingCompanyId(playerCompanyId);

            Ui::WindowManager::closeAllFloatingWindows();

            GameCommands::setUpdatingCompanyId(previousUpdatingId);
            SceneManager::addSceneFlags(SceneManager::Flags::networked);

            // If the other party is leaving the game, go back to the title screen.
            if (playerCompanyId != previousUpdatingId)
            {
                // 0x0043C1CD
                SceneManager::removeSceneFlags(SceneManager::Flags::networked);
                SceneManager::removeSceneFlags(SceneManager::Flags::networkHost);
                CompanyManager::setControllingId(CompanyId(0));
                CompanyManager::setSecondaryPlayerId(CompanyId::null);

                Gfx::invalidateScreen();
                ObjectManager::loadIndex();

                Ui::WindowManager::close(Ui::WindowType::options);
                Ui::WindowManager::close(Ui::WindowType::companyFaceSelection);
                Ui::WindowManager::close(Ui::WindowType::objectSelection);

                SceneManager::removeSceneFlags(SceneManager::Flags::editor);
                Audio::pauseSound();
                Audio::unpauseSound();

                if (Input::hasFlag(Input::Flags::rightMousePressed))
                {
                    Input::stopCursorDrag();
                    Input::resetFlag(Input::Flags::rightMousePressed);
                }

                Title::start();

                Ui::Windows::Error::open(StringIds::error_the_other_player_has_exited_the_game);

                throw GameException::Interrupt;
            }
        }

        // 0x0043C1C8
        exitCleanly();
    }

    // 0x0043C0FD
    void returnToTitle()
    {
        if (SceneManager::isNetworked())
        {
            Ui::WindowManager::closeAllFloatingWindows();
        }

        Ui::WindowManager::close(Ui::WindowType::options);
        Ui::WindowManager::close(Ui::WindowType::companyFaceSelection);
        Ui::WindowManager::close(Ui::WindowType::objectSelection);
        Ui::WindowManager::close(Ui::WindowType::saveGamePrompt);

        SceneManager::removeSceneFlags(SceneManager::Flags::editor);
        Audio::pauseSound();
        Audio::unpauseSound();

        if (Input::hasFlag(Input::Flags::rightMousePressed))
        {
            Input::stopCursorDrag();
            Input::resetFlag(Input::Flags::rightMousePressed);
        }

        Title::start();

        throw GameException::Interrupt;
    }

    // 0x0043C427
    void confirmSaveGame(LoadOrQuitMode promptSaveType)
    {
        ToolManager::toolCancel();

        if (SceneManager::isEditorMode())
        {
            if (auto res = Game::saveLandscapeOpen())
            {
                if (saveLandscape(*res))
                {
                    // load landscape
                    GameCommands::LoadSaveQuitGameArgs args{};
                    args.loadQuitMode = LoadOrQuitMode::loadGamePrompt;
                    args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::dontSave;
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                }
            }
        }
        else if (!SceneManager::isNetworked())
        {
            if (auto res = Game::saveSaveGameOpen())
            {
                // 0x0043C446
                auto path = fs::u8path(*res).replace_extension(S5::extensionSV5);
                _activeSavePath = path.u8string();

                S5::SaveFlags flags = S5::SaveFlags::none;
                if (Config::get().exportObjectsWithSaves)
                {
                    flags = S5::SaveFlags::packCustomObjects;
                }

                if (!S5::exportGameStateToFile(path, flags))
                {
                    Ui::Windows::Error::open(StringIds::error_game_save_failed, StringIds::null);
                }
                else
                {
                    GameCommands::LoadSaveQuitGameArgs args{};
                    args.loadQuitMode = LoadOrQuitMode::loadGamePrompt;
                    args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::dontSave;
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                }
            }
        }
        else
        {
            // 0x0043C511
            GameCommands::do_72();
            MultiPlayer::setFlag(MultiPlayer::flags::flag_2);

            switch (promptSaveType)
            {
                case LoadOrQuitMode::loadGamePrompt:
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_13); // intend to load?
                    break;
                case LoadOrQuitMode::returnToTitlePrompt:
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_14); // intend to return to title?
                    break;
                case LoadOrQuitMode::quitGamePrompt:
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_15); // intend to quit game?
                    break;
            }
        }

        // 0x0043C411
        Gfx::invalidateScreen();
    }

    bool saveLandscape(std::string filename)
    {
        // 0x0043C4B3
        auto path = fs::u8path(filename).replace_extension(S5::extensionSC5);
        _activeSavePath = path.u8string();

        bool saveResult = !S5::exportGameStateToFile(path, S5::SaveFlags::scenario);
        if (saveResult)
        {
            Ui::Windows::Error::open(StringIds::landscape_save_failed);
        }

        return saveResult;
    }

    std::string getActiveSavePath()
    {
        return _activeSavePath;
    }

    void setActiveSavePath(std::string path)
    {
        _activeSavePath = path;
    }

    GameStateFlags getFlags()
    {
        return getGameState().flags;
    }

    void setFlags(GameStateFlags flags)
    {
        getGameState().flags = flags;
    }

    bool hasFlags(GameStateFlags flags)
    {
        return (getFlags() & flags) != GameStateFlags::none;
    }

    void removeFlags(GameStateFlags flags)
    {
        setFlags(getFlags() & ~flags);
    }
}
