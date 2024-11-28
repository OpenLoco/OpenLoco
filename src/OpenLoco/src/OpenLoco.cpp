#include "CommandLine.h"
#include "Scenario.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <setjmp.h>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
// timeGetTime is unavailable if we use lean and mean
// #define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <windows.h>

// `small` is used as a type in `windows.h`
#undef small
#endif

#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "EditorController.h"
#include "Effects/EffectsManager.h"
#include "Entities/EntityManager.h"
#include "Entities/EntityTweener.h"
#include "Environment.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameException.hpp"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Gui.h"
#include "Input.h"
#include "Input/Shortcuts.h"
#include "Interop/Hooks.h"
#include "Intro.h"
#include "Localisation/Formatting.h"
#include "Localisation/LanguageFiles.h"
#include "Localisation/Languages.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/AnimationManager.h"
#include "Map/TileManager.h"
#include "Map/WaveManager.h"
#include "MessageManager.h"
#include "MultiPlayer.h"
#include "Network/Network.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Random.h"
#include "S5/S5.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "Title.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Crash.h>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Utility/String.hpp>

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Input;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco
{
    using Clock = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;

    static double _accumulator = 0.0;
    static Timepoint _lastUpdate = Clock::now();
    static CrashHandler::Handle _exHandler = nullptr;

    static loco_global<uint16_t, 0x0050C19C> _time_since_last_tick;
    static loco_global<uint32_t, 0x0050C19E> _last_tick_time;

    static loco_global<int8_t, 0x0052336E> _52336E; // bool

    static int32_t _monthsSinceLastAutosave;

    static void autosaveReset();
    static void tickLogic(int32_t count);
    static void tickLogic();
    static void dateTick();

    std::string getVersionInfo()
    {
        return version;
    }

    // 0x00407FFD
    static bool isAlreadyRunning(const char* mutexName)
    {
        auto result = ((int32_t(*)(const char*))(0x00407FFD))(mutexName);
        return result != 0;
    }

    // 0x004BE621
    [[noreturn]] void exitWithError(StringId eax, StringId ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        call(0x004BE621, regs);
        exitCleanly();
    }

    // 0x004BE5EB
    [[noreturn]] void exitWithError(StringId message, uint32_t errorCode)
    {
        // Saves the error code for later writing to error log 1.TMP.
        registers regs;
        regs.eax = errorCode;
        regs.bx = message;
        call(0x004BE5EB, regs);
        exitCleanly();
    }

    // 0x004BE65E
    [[noreturn]] void exitCleanly()
    {
        Audio::disposeDSound();
        Audio::close();
        Ui::disposeCursors();
        Localisation::unloadLanguageFile();

        auto tempFilePath = Environment::getPathNoWarning(Environment::PathId::_1tmp);
        if (fs::exists(tempFilePath))
        {
            auto path8 = tempFilePath.u8string();
            Logging::info("Removing temp file '{}'", path8.c_str());
            fs::remove(tempFilePath);
        }
        CrashHandler::shutdown(_exHandler);

        // Logging should be the last before terminating.
        Logging::shutdown();

        // SDL_Quit();
        exit(0);
    }

    // 0x00441400
    static void startupChecks()
    {
        const auto& config = Config::get();
        if (!config.allowMultipleInstances && isAlreadyRunning("Locomotion"))
        {
            exitWithError(StringIds::game_init_failure, StringIds::loco_already_running);
        }

        // Originally the game would check that all the game
        // files exist are some have the correct checksum. We
        // do not need to do this anymore, the game should work
        // with g1 alone and some objects?
    }

    // 0x004C57C0
    void initialiseViewports()
    {
        Ui::Windows::MapToolTip::reset();

        Colours::initColourMap();
        Ui::WindowManager::init();
        Ui::ViewportManager::init();

        Input::init();
        Input::initMouse();

        // tooltip-related
        _52336E = 0;

        Ui::Windows::TextInput::cancel();

        // TODO Move this to a more generic, initialise game state function when
        //      we have one hooked / implemented.
        autosaveReset();
    }

    static void initialise()
    {
        _last_tick_time = Platform::getTime();

        std::srand(std::time(nullptr));
        addr<0x0050C18C, int32_t>() = addr<0x00525348, int32_t>();

        World::TileManager::allocateMapElements();
        Environment::resolvePaths();
        Localisation::enumerateLanguages();
        Localisation::loadLanguageFile();
        startupChecks();

        Gfx::loadG1();
        Gfx::initialise();

        Ui::initialise();
        Ui::initialiseCursors();
        initialiseViewports();
        Gui::init();

        MessageManager::reset();
        Scenario::reset();

        setScreenFlag(ScreenFlags::initialised);

        ObjectManager::loadIndex();
        ScenarioManager::loadIndex();

        const auto& cmdLineOptions = getCommandLineOptions();
        if (cmdLineOptions.action == CommandLineAction::intro)
        {
            Intro::state(Intro::State::begin);
        }
        else
        {
            Intro::state(Intro::State::end);
        }

        Title::start();
    }

    static void loadFile(const fs::path& path)
    {
        auto extension = path.extension().u8string();
        if (Utility::iequals(extension, S5::extensionSC5))
        {
            Scenario::loadAndStart(path);
        }
        else
        {
            S5::importSaveToGameState(path, S5::LoadFlags::none);
        }
    }

    static void loadFile(const std::string& path)
    {
        loadFile(fs::u8path(path));
    }

    static void launchGameFromCmdLineOptions()
    {
        const auto& cmdLineOptions = getCommandLineOptions();
        if (cmdLineOptions.action == CommandLineAction::host)
        {
            Network::openServer();
            loadFile(cmdLineOptions.path);
        }
        else if (cmdLineOptions.action == CommandLineAction::join)
        {
            if (cmdLineOptions.port)
            {
                Network::joinServer(cmdLineOptions.address, *cmdLineOptions.port);
            }
            else
            {
                Network::joinServer(cmdLineOptions.address);
            }
        }
        else if (!cmdLineOptions.path.empty())
        {
            loadFile(cmdLineOptions.path);
        }
    }

    void sub_431695(uint16_t var_F253A0)
    {
        GameCommands::setUpdatingCompanyId(CompanyManager::getControllingId());
        for (auto i = 0; i < var_F253A0; i++)
        {
            MessageManager::sub_428E47();
            WindowManager::dispatchUpdateAll();
        }

        Input::processKeyboardInput();
        WindowManager::update();
        Ui::handleInput();
        CompanyManager::updateOwnerStatus();
    }

    // This is called when the game requested to end the current tick early.
    // This can be caused by loading a new save game or exceptions.
    static void tickInterrupted()
    {
        EntityTweener::get().reset();
        Logging::info("Tick interrupted");
    }

    // 0x0046A794
    static void tick()
    {
        try
        {
            addr<0x00113E87C, int32_t>() = 0;
            addr<0x0005252E0, int32_t>() = 0;

            uint32_t time = Platform::getTime();
            _time_since_last_tick = (uint16_t)std::min(time - _last_tick_time, 500U);
            _last_tick_time = time;

            if (!isPaused())
            {
                addr<0x0050C1A2, uint32_t>() += _time_since_last_tick;
            }
            if (Tutorial::state() != Tutorial::State::none)
            {
                _time_since_last_tick = 31;
            }

            GameCommands::resetCommandNestLevel();
            Ui::update();

            addr<0x005233AE, int32_t>() += addr<0x0114084C, int32_t>();
            addr<0x005233B2, int32_t>() += addr<0x01140840, int32_t>();
            addr<0x0114084C, int32_t>() = 0;
            addr<0x01140840, int32_t>() = 0;

            {
                call(0x00440DEC); // install scenario from 0x0050C18C ptr??

                if (addr<0x00525340, int32_t>() == 1)
                {
                    addr<0x00525340, int32_t>() = 0;
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_1);
                }

                Input::handleKeyboard();
                Audio::updateSounds();

                Network::update();

                addr<0x0050C1AE, int32_t>()++;
                if (Intro::isActive())
                {
                    Intro::update();
                    if (!Intro::isActive())
                    {
                        launchGameFromCmdLineOptions();
                    }
                }
                else
                {
                    uint16_t numUpdates = std::clamp<uint16_t>(_time_since_last_tick / (uint16_t)31, 1, 3);
                    if (WindowManager::find(Ui::WindowType::multiplayer, 0) != nullptr)
                    {
                        numUpdates = 1;
                    }
                    if (isNetworked())
                    {
                        numUpdates = 1;
                    }
                    if (addr<0x00525324, int32_t>() == 1)
                    {
                        addr<0x00525324, int32_t>() = 0;
                        numUpdates = 1;
                    }
                    else
                    {
                        switch (Input::state())
                        {
                            case State::reset:
                            case State::normal:
                            case State::dropdownActive:
                                if (Input::hasFlag(Flags::viewportScrolling))
                                {
                                    Input::resetFlag(Flags::viewportScrolling);
                                    numUpdates = 1;
                                }
                                break;
                            case State::widgetPressed: break;
                            case State::positioningWindow: break;
                            case State::viewportRight: break;
                            case State::viewportLeft: break;
                            case State::scrollLeft: break;
                            case State::resizing: break;
                            case State::scrollRight: break;
                        }
                    }

                    Ui::WindowManager::setVehiclePreviewRotationFrame(Ui::WindowManager::getVehiclePreviewRotationFrame() + numUpdates);

                    if (isPaused())
                    {
                        numUpdates = 0;
                    }
                    uint16_t var_F253A0 = std::max<uint16_t>(1, numUpdates);
                    setScreenAge(std::min(0xFFFF, (int32_t)getScreenAge() + var_F253A0));
                    if (getGameSpeed() != GameSpeed::Normal)
                    {
                        numUpdates *= 3;
                        if (getGameSpeed() != GameSpeed::FastForward)
                        {
                            numUpdates *= 3;
                        }
                    }

                    // Catch up to server (usually after we have just joined the game)
                    auto numTicksBehind = Network::getServerTick() - ScenarioManager::getScenarioTicks();
                    if (numTicksBehind > 4)
                    {
                        numUpdates = 4;
                    }

                    tickLogic(numUpdates);

                    getGameState().var_014A++;
                    if (isEditorMode())
                    {
                        EditorController::tick();
                    }

                    Audio::playBackgroundMusic();

                    // 0x0052532C != 0 isMinimized
                    if (Tutorial::state() != Tutorial::State::none && addr<0x0052532C, int32_t>() != 0 && Ui::width() < 64)
                    {
                        Tutorial::stop();

                        // This ends with a premature tick termination
                        Game::returnToTitle();
                        return; // won't be reached
                    }

                    sub_431695(var_F253A0);

                    if (Config::get().old.countdown != 0xFF)
                    {
                        Config::get().old.countdown++;
                        if (Config::get().old.countdown != 0xFF)
                        {
                            Config::write();
                        }
                    }
                }
            }
        }
        catch (GameException)
        {
            // Premature end of current tick; use a different message to indicate it's from C++ code
            tickInterrupted();
            return;
        }
    }

    static void tickLogic(int32_t count)
    {
        for (int32_t i = 0; i < count; i++)
        {
            tickLogic();
        }
    }

    static loco_global<int8_t, 0x0050C197> _loadErrorCode;
    static loco_global<StringId, 0x0050C198> _loadErrorMessage;

    // 0x0046ABCB
    static void tickLogic()
    {
        if (!Network::shouldProcessTick(ScenarioManager::getScenarioTicks() + 1))
        {
            return;
        }

        ScenarioManager::setScenarioTicks(ScenarioManager::getScenarioTicks() + 1);
        ScenarioManager::setScenarioTicks2(ScenarioManager::getScenarioTicks2() + 1);
        Network::processGameCommands(ScenarioManager::getScenarioTicks());

        recordTickStartPrng();
        World::TileManager::defragmentTilePeriodic();
        addr<0x00F25374, uint8_t>() = S5::getOptions().madeAnyChanges;
        dateTick();
        World::TileManager::update();
        World::WaveManager::update();
        TownManager::update();
        IndustryManager::update();
        VehicleManager::update();
        StationManager::update();
        EffectsManager::update();
        CompanyManager::update();
        World::AnimationManager::update();
        Audio::updateVehicleNoise();
        Audio::updateAmbientNoise();
        Title::update();

        S5::getOptions().madeAnyChanges = addr<0x00F25374, uint8_t>();
        if (_loadErrorCode != 0)
        {
            if (_loadErrorCode == -2)
            {
                StringId title = _loadErrorMessage;
                StringId message = StringIds::null;
                Ui::Windows::Error::open(title, message);
            }
            else
            {
                auto objectList = S5::getObjectErrorList();
                Ui::Windows::ObjectLoadError::open(objectList);
            }
            _loadErrorCode = 0;
        }
    }

    static void autosaveReset()
    {
        _monthsSinceLastAutosave = 0;
    }

    static void autosaveClean()
    {
        try
        {
            auto autosaveDirectory = Environment::getPath(Environment::PathId::autosave);
            if (fs::is_directory(autosaveDirectory))
            {
                std::vector<fs::path> autosaveFiles;

                // Collect all the autosave files
                for (auto& f : fs::directory_iterator(autosaveDirectory))
                {
                    if (f.is_regular_file())
                    {
                        auto& path = f.path();
                        auto filename = path.filename().u8string();
                        if (Utility::startsWith(filename, "autosave_") && Utility::endsWith(filename, S5::extensionSV5, true))
                        {
                            autosaveFiles.push_back(path);
                        }
                    }
                }

                auto amountToKeep = static_cast<size_t>(std::max(1, Config::get().autosaveAmount));
                if (autosaveFiles.size() > amountToKeep)
                {
                    // Sort them by name (which should correspond to date order)
                    std::sort(autosaveFiles.begin(), autosaveFiles.end());

                    // Delete excess files
                    auto numToDelete = autosaveFiles.size() - amountToKeep;
                    for (size_t i = 0; i < numToDelete; i++)
                    {
                        auto path8 = autosaveFiles[i].u8string();
                        Logging::info("Deleting old autosave: {}", path8.c_str());
                        fs::remove(autosaveFiles[i]);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to clean autosaves: {}", e.what());
        }
    }

    static void autosave()
    {
        // Format filename
        auto time = std::time(nullptr);
        auto localTime = std::localtime(&time);
        char filename[64];
        snprintf(
            filename,
            sizeof(filename),
            "autosave_%04u-%02u-%02u_%02u-%02u-%02u%s",
            localTime->tm_year + 1900,
            localTime->tm_mon + 1,
            localTime->tm_mday,
            localTime->tm_hour,
            localTime->tm_min,
            localTime->tm_sec,
            S5::extensionSV5);

        try
        {
            auto autosaveDirectory = Environment::getPath(Environment::PathId::autosave);
            Environment::autoCreateDirectory(autosaveDirectory);

            auto autosaveFullPath = autosaveDirectory / filename;

            auto autosaveFullPath8 = autosaveFullPath.u8string();
            Logging::info("Autosaving game to {}", autosaveFullPath8.c_str());
            S5::exportGameStateToFile(autosaveFullPath, S5::SaveFlags::isAutosave | S5::SaveFlags::noWindowClose);
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to autosave game: {}", e.what());
        }
    }

    static void autosaveCheck()
    {
        _monthsSinceLastAutosave++;

        if (!isTitleMode())
        {
            auto freq = Config::get().autosaveFrequency;
            if (freq > 0 && _monthsSinceLastAutosave >= freq)
            {
                autosave();
                autosaveClean();
                autosaveReset();
            }
        }
    }

    // 0x004968C7
    static void dateTick()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
        {
            if (updateDayCounter())
            {
                StationManager::updateDaily();
                VehicleManager::updateDaily();
                IndustryManager::updateDaily();
                MessageManager::updateDaily();
                WindowManager::updateDaily();

                auto yesterday = calcDate(getCurrentDay() - 1);
                auto today = calcDate(getCurrentDay());
                setDate(today);
                Scenario::updateSnowLine(today.dayOfOlympiad);
                Ui::Windows::TimePanel::invalidateFrame();

                if (today.month != yesterday.month)
                {
                    // End of every month
                    Scenario::getObjectiveProgress().monthsInChallenge++;
                    TownManager::updateMonthly();
                    IndustryManager::updateMonthly();
                    CompanyManager::updateMonthly1();
                    CompanyManager::updateMonthlyHeadquarters();
                    VehicleManager::updateMonthly();

                    if (today.year <= 2029)
                    {
                        Economy::updateMonthly();
                    }

                    // clang-format off
                    if (today.month == MonthId::january ||
                        today.month == MonthId::april ||
                        today.month == MonthId::july ||
                        today.month == MonthId::october)
                    // clang-format on
                    {
                        CompanyManager::updateQuarterly();
                    }

                    if (today.year != yesterday.year)
                    {
                        // End of every year
                        CompanyManager::updateYearly();
                        ObjectManager::updateDefaultLevelCrossingType();
                        ObjectManager::updateYearly2();
                        World::TileManager::updateYearly();
                    }

                    autosaveCheck();
                }

                CompanyManager::updateDaily();
            }
        }
    }

    static void tickWait()
    {
        // Idle loop for a 40 FPS
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (Platform::getTime() - _last_tick_time < Engine::UpdateRateInMs);
    }

    bool promptTickLoop(std::function<bool()> tickAction)
    {
        while (true)
        {
            _last_tick_time = Platform::getTime();
            _time_since_last_tick = 31;
            if (!Ui::processMessages())
            {
                return false;
            }
            if (!tickAction())
            {
                break;
            }
            Ui::render();
            tickWait();
        }
        return true;
    }

    constexpr auto MaxUpdateTime = static_cast<double>(Engine::MaxTimeDeltaMs) / 1000.0;
    constexpr auto UpdateTime = static_cast<double>(Engine::UpdateRateInMs) / 1000.0;
    constexpr auto TimeScale = 1.0;

    static void variableUpdate()
    {
        auto& tweener = EntityTweener::get();

        const auto alpha = std::min<float>(_accumulator / UpdateTime, 1.0);

        while (_accumulator > UpdateTime)
        {
            tweener.preTick();

            tick();
            _accumulator -= UpdateTime;

            tweener.postTick();
        }

        tweener.tween(alpha);

        Ui::render();
    }

    static void fixedUpdate()
    {
        auto& tweener = EntityTweener::get();
        tweener.reset();

        if (_accumulator < UpdateTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else
        {
            tick();
            _accumulator -= UpdateTime;

            Ui::render();
        }
    }

    static void update()
    {
        auto timeNow = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(timeNow - _lastUpdate).count() / 1'000'000.0;

        elapsed *= TimeScale;

        _accumulator = std::min(_accumulator + elapsed, MaxUpdateTime);
        _lastUpdate = timeNow;

        if (Config::get().uncapFPS)
        {
            variableUpdate();
        }
        else
        {
            fixedUpdate();
        }
    }

    // 0x00406386
    static void run()
    {
#ifdef _WIN32
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
        initialise();

        while (Ui::processMessages())
        {
            update();
        }

#ifdef _WIN32
        CoUninitialize();
#endif
    }

    /**
     * We do our own command line logic, but we still execute routines that try to read lpCmdLine,
     * so make sure it is initialised to a pointer to an empty string. Remove this when no more
     * original code is called that uses lpCmdLine (e.g. 0x00440DEC)
     */
    static void resetCmdline()
    {
        loco_global<const char*, 0x00525348> _glpCmdLine;
        _glpCmdLine = "";
    }

    void simulateGame(const fs::path& savePath, int32_t ticks)
    {
        Config::read();

        if (getCommandLineOptions().locomotionDataPath.has_value())
        {
            auto& cfg = Config::get();
            cfg.locoInstallPath = getCommandLineOptions().locomotionDataPath.value();
        }

        Environment::resolvePaths();
        resetCmdline();
        registerHooks();

        try
        {
            initialise();
            loadFile(savePath);
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to simulate park: {}", e.what());
        }
        catch (const GameException i)
        {
            if (i != GameException::Interrupt)
            {
                Logging::error("Unable to simulate park!");
            }
            else
            {
                Logging::info("File loaded. Starting simulation.");
            }
        }
        tickLogic(ticks);
    }

    // 0x00406D13
    static int main(const CommandLineOptions& options)
    {
        // Bootstrap the logging system.
        Logging::initialize(options.logLevels);

        // Always print the product name and version first.
        Logging::info("{}", OpenLoco::getVersionInfo());

        Environment::setLocale();

        auto ret = runCommandLineOnlyCommand(options);
        if (ret)
        {
            return *ret;
        }

        setCommandLineOptions(options);

        if (!OpenLoco::Platform::isRunningInWine())
        {
            CrashHandler::AppInfo appInfo;
            appInfo.name = "OpenLoco";
            appInfo.version = getVersionInfo();

            _exHandler = CrashHandler::init(appInfo);
        }
        else
        {
            Logging::warn("Detected wine, not installing crash handler as it doesn't provide useful data. Consider using native builds of OpenLoco instead.");
        }

        try
        {
            Input::Shortcuts::initialize();

            const auto& cfg = Config::read();
            Environment::resolvePaths();

            resetCmdline();
            registerHooks();

            Ui::createWindow(cfg.display);
            call(0x004078FE); // getSystemInfo used for some config, multiplayer name,
            Audio::initialiseDSound();
            run();
            exitCleanly();
        }
        catch (const std::exception& e)
        {
            Logging::error("Exception: {}", e.what());
            Ui::showMessageBox("Exception", e.what());
            exitCleanly();
        }
        catch (...)
        {
            Ui::showMessageBox("Exception", "Unsure what threw the exception!");
            exitCleanly();
        }
    }

    int main(std::vector<std::string>&& argv)
    {
        auto options = parseCommandLine(std::move(argv));
        if (options)
        {
            return main(*options);
        }
        else
        {
            return 1;
        }
    }
}
