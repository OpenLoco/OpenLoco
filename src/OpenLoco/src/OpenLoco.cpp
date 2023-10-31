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
#include "Drawing/SoftwareDrawingEngine.h"
#include "Economy/Economy.h"
#include "EditorController.h"
#include "Effects/EffectsManager.h"
#include "Entities/EntityManager.h"
#include "Entities/EntityTweener.h"
#include "Environment.h"
#include "Game.h"
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

    static std::unique_ptr<S5::S5File> refFile;

    loco_global<char[256], 0x005060D0> _gCDKey;

    loco_global<uint16_t, 0x0050C19C> _time_since_last_tick;
    loco_global<uint32_t, 0x0050C19E> _last_tick_time;
    loco_global<uint8_t, 0x00508F08> _game_command_nest_level;
    static loco_global<StringId, 0x0050A018> _mapTooltipFormatArguments;
    static loco_global<int32_t, 0x0052339C> _52339C;
    static loco_global<int8_t, 0x0052336E> _52336E; // bool

    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;

    static loco_global<char[256], 0x011367A0> _11367A0;
    static loco_global<char[256], 0x011368A0> _11368A0;

    static int32_t _monthsSinceLastAutosave;

    static void autosaveReset();
    static void tickLogic(int32_t count);
    static void tickLogic();
    static void dateTick();
    static void sub_46FFCA();

    std::string getVersionInfo()
    {
        return version;
    }

#ifdef _NO_LOCO_WIN32_
    /**
     * Use this to allocate memory that will be freed in vanilla code or via loco_free.
     */
    [[maybe_unused]] static void* malloc(size_t size)
    {
        return ((void* (*)(size_t))0x004D1401)(size);
    }

    /**
     * Use this to reallocate memory that will be freed in vanilla code or via loco_free.
     */
    [[maybe_unused]] static void* realloc(void* address, size_t size)
    {
        return ((void* (*)(void*, size_t))0x004D1B28)(address, size);
    }

    /**
     * Use this to free up memory allocated in vanilla code or via loco_malloc / loco_realloc.
     */
    [[maybe_unused]] static void free(void* address)
    {
        ((void (*)(void*))0x004D1355)(address);
    }
#endif // _NO_LOCO_WIN32_

    static void sub_4062D1()
    {
        call(0x004062D1); // calls getTime then sub_4062E0 unused Dead code
    }

    static void sub_406417(void* ptr)
    {
        ((void (*)(void*))0x00406417)(ptr);
    }

    static void sub_40567E()
    {
        call(0x0040567E);
    }

    static void sub_4058F5()
    {
        call(0x004058F5);
    }

    static void sub_4062E0()
    {
        call(0x004062E0); // getTime unused Dead code
    }

    static bool sub_4034FC(int32_t& a, int32_t& b)
    {
        auto result = ((int32_t(*)(int32_t&, int32_t&))(0x004034FC))(a, b);
        return result != 0;
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
        Ui::disposeInput();
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

        // rain-related
        _52339C = -1;

        // tooltip-related
        _52336E = 0;

        Ui::Windows::TextInput::cancel();

        StringManager::formatString(_11367A0, StringIds::label_button_ok);
        StringManager::formatString(_11368A0, StringIds::label_button_cancel);

        // TODO Move this to a more generic, initialise game state function when
        //      we have one hooked / implemented.
        autosaveReset();
    }

    static void initialise()
    {
        std::srand(std::time(nullptr));
        addr<0x0050C18C, int32_t>() = addr<0x00525348, int32_t>();
        call(0x004078BE); // getSystemTime unused dead code?
        World::TileManager::allocateMapElements();
        Environment::resolvePaths();
        Localisation::enumerateLanguages();
        Localisation::loadLanguageFile();
        Ui::ProgressBar::begin(StringIds::loading);
        Ui::ProgressBar::setProgress(30);
        startupChecks();
        Ui::ProgressBar::setProgress(40);
        Ui::ProgressBar::end();
        ObjectManager::loadIndex();
        ScenarioManager::loadIndex();
        Ui::ProgressBar::begin(StringIds::loading);
        Ui::ProgressBar::setProgress(60);
        Gfx::loadG1();
        Ui::ProgressBar::setProgress(220);
        Gfx::initialiseCharacterWidths();
        Gfx::initialiseNoiseMaskMap();
        Ui::ProgressBar::setProgress(235);
        Ui::ProgressBar::setProgress(250);
        Ui::initialiseCursors();
        Ui::ProgressBar::end();
        Ui::initialise();
        initialiseViewports();
        Title::sub_4284C8();
        call(0x004969DA); // getLocalTime not used (dead code?)
        Scenario::reset();
        setScreenFlag(ScreenFlags::initialised);
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
        Gui::init();
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.clear(Gfx::getScreenRT(), 0x0A0A0A0A);
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

    static void launchGame()
    {
        const auto& cmdLineOptions = getCommandLineOptions();
        if (cmdLineOptions.action == CommandLineAction::host)
        {
            Network::openServer();
            loadFile(cmdLineOptions.path);
        }
        else if (cmdLineOptions.action == CommandLineAction::join)
        {
            Title::start();
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
        else
        {
            Title::start();
        }
    }

    // 0x0046E388
    static void sub_46E388()
    {
        call(0x0046E388);
    }

    // 0x004317BD
    static uint32_t sub_4317BD()
    {
        registers regs;
        call(0x004317BD, regs);

        return regs.eax;
    }

    // 0x0046E4E3
    static void sub_46E4E3()
    {
        call(0x0046E4E3);
    }

    void sub_431695(uint16_t var_F253A0)
    {
        _updatingCompanyId = CompanyManager::getControllingId();
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

    [[maybe_unused]] static void old_sub_431695(uint16_t var_F253A0)
    {
        if (!isNetworked())
        {
            _updatingCompanyId = CompanyManager::getControllingId();
            for (auto i = 0; i < var_F253A0; i++)
            {
                MessageManager::sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            Ui::handleInput();
            CompanyManager::updateOwnerStatus();
            return;
        }

        // Only run every other tick?
        if (getGameState().var_014A % 2 != 0)
        {
            return;
        }

        // Host/client?
        if (isNetworkHost())
        {
            _updatingCompanyId = CompanyManager::getControllingId();

            // run twice as often as var_F253A0
            for (auto i = 0; i < var_F253A0 * 2; i++)
            {
                MessageManager::sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            WindowManager::update();
            Ui::handleInput();
            CompanyManager::updateOwnerStatus();
            sub_46E388();

            _updatingCompanyId = CompanyManager::getSecondaryPlayerId();
            sub_4317BD();
        }
        else
        {
            _updatingCompanyId = CompanyManager::getSecondaryPlayerId();
            auto eax = sub_4317BD();

            _updatingCompanyId = CompanyManager::getControllingId();
            if (!isTitleMode())
            {
                auto edx = gPrng1().srand_0();
                edx ^= CompanyManager::get(CompanyId(0))->cash.var_00;
                edx ^= CompanyManager::get(CompanyId(1))->cash.var_00;
                if (edx != eax)
                {
                    // disconnect?
                    sub_46E4E3();
                    return;
                }
            }

            // run twice as often as var_F253A0
            for (auto i = 0; i < var_F253A0 * 2; i++)
            {
                MessageManager::sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            WindowManager::update();
            Ui::handleInput();
            CompanyManager::updateOwnerStatus();
            sub_46E388();
        }
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
        static bool isInitialised = false;

        // Locomotion has several routines that will prematurely end the current tick.
        // This usually happens when switching game mode. It does this by jumping to
        // the end of the original routine and resetting esp back to an initial value
        // stored at the beginning of tick. Until those routines are re-written, we
        // must simulate it using 'setjmp'.
        static jmp_buf tickJump;

        // When Locomotion wants to jump to the end of a tick, it sets ESP
        // to some static memory that we define
        static loco_global<void*, 0x0050C1A6> _tickJumpESP;
        static uint8_t spareStackMemory[2048];
        _tickJumpESP = spareStackMemory + sizeof(spareStackMemory);

        if (setjmp(tickJump))
        {
            // Premature end of current tick
            tickInterrupted();
            return;
        }

        try
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            addr<0x00113E87C, int32_t>() = 0;
            addr<0x0005252E0, int32_t>() = 0;
            if (!isInitialised)
            {
                isInitialised = true;

                // This address is where those routines jump back to to end the tick prematurely
                registerHook(
                    0x0046AD71,
                    [](registers&) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                        longjmp(tickJump, 1);
                    });

                initialise();
                _last_tick_time = Platform::getTime();
            }

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
            _game_command_nest_level = 0;
            Ui::update();

            addr<0x005233AE, int32_t>() += addr<0x0114084C, int32_t>();
            addr<0x005233B2, int32_t>() += addr<0x01140840, int32_t>();
            addr<0x0114084C, int32_t>() = 0;
            addr<0x01140840, int32_t>() = 0;
            if (Config::get().old.var_72 == 0)
            {
                Config::get().old.var_72 = 16;
                const auto cursor = Ui::getCursorPosScaled();
                addr<0x00F2538C, Ui::Point32>() = cursor;
                drawingCtx.clear(Gfx::getScreenRT(), 0);
                addr<0x00F2539C, int32_t>() = 0;
            }
            else
            {
                if (Config::get().old.var_72 >= 16)
                {
                    Config::get().old.var_72++;
                    if (Config::get().old.var_72 >= 48)
                    {
                        if (sub_4034FC(addr<0x00F25394, int32_t>(), addr<0x00F25398, int32_t>()))
                        {
                            uintptr_t esi = addr<0x00F25390, int32_t>() + 4;
                            esi *= addr<0x00F25398, int32_t>();
                            esi += addr<0x00F2538C, int32_t>();
                            esi += 2;
                            esi += addr<0x00F25394, int32_t>();
                            addr<0x00F2539C, int32_t>() |= *((int32_t*)esi);
                            call(0x00403575); // ddrwaUnlockPSurface
                        }
                    }
                    Ui::setCursorPosScaled(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                    Gfx::invalidateScreen();
                    if (Config::get().old.var_72 != 96)
                    {
                        return;
                    }
                    Config::get().old.var_72 = 1;
                    if (addr<0x00F2539C, int32_t>() != 0)
                    {
                        Config::get().old.var_72 = 2;
                    }
                    Config::write();
                }

                call(0x00452D1A); // nop redrawPeepAndRain
                call(0x00440DEC);

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
                        launchGame();
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

                    sub_46FFCA();

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

                    if (Tutorial::state() != Tutorial::State::none && addr<0x0052532C, int32_t>() != 0 && addr<0x0113E2E4, int32_t>() < 0x40)
                    {
                        Tutorial::stop();

                        // This ends with a premature tick termination
                        Game::returnToTitle();
                        return; // won't be reached
                    }

                    sub_431695(var_F253A0);
                    call(0x00452B5F); // nop was updateRainAnimation
                    sub_46FFCA();
                    if (Config::get().old.countdown != 0xFF)
                    {
                        Config::get().old.countdown++;
                        if (Config::get().old.countdown != 0xFF)
                        {
                            Config::write();
                        }
                    }
                }

                if (Config::get().old.var_72 == 2)
                {
                    addr<0x005252DC, int32_t>() = 1;
                    const auto cursor = Ui::getCursorPosScaled();
                    addr<0x00F2538C, Ui::Point32>() = cursor;
                    Ui::setCursorPosScaled(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
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

    static void sub_46FFCA()
    {
        addr<0x010E7D3C, uint32_t>() = 0x2A0015;
        addr<0x010E7D40, uint32_t>() = 0x210026;
        addr<0x010E7D44, uint32_t>() = 0x5C2001F;
        addr<0x010E7D48, uint32_t>() = 0xFFFF0019;
        addr<0x010E7D4C, uint32_t>() = 0xFFFFFFFF;
        addr<0x010E7D50, uint32_t>() = 0x1AFFFF;
        addr<0x010E7D54, uint32_t>() = 0xFFFFFFFF;
        addr<0x010E7D58, uint32_t>() = 0xFFFF001B;
        addr<0x010E7D5C, uint32_t>() = 0x64700A3;
        addr<0x010E7D60, uint32_t>() = 0xCE0481;
        addr<0x010E7D64, uint32_t>() = 0xD900BF;
    }

    static loco_global<int8_t, 0x0050C197> _loadErrorCode;
    static loco_global<StringId, 0x0050C198> _loadErrorMessage;

    // 0x0046ABCB
    static void tickLogic()
    {
        if (!Network::shouldProcessTick(ScenarioManager::getScenarioTicks() + 1))
            return;

        ScenarioManager::setScenarioTicks(ScenarioManager::getScenarioTicks() + 1);
        ScenarioManager::setScenarioTicks2(ScenarioManager::getScenarioTicks2() + 1);
        Network::processGameCommands(ScenarioManager::getScenarioTicks());

        recordTickStartPrng();
        call(0x004613F0); // Map::TileManager::reorg?
        addr<0x00F25374, uint8_t>() = S5::getOptions().madeAnyChanges;
        dateTick();
        World::TileManager::update();
        World::WaveManager::update();
        TownManager::update();
        IndustryManager::update();
        VehicleManager::update();
        sub_46FFCA();
        StationManager::update();
        EffectsManager::update();
        sub_46FFCA();
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
            S5::exportGameStateToFile(autosaveFullPath, S5::SaveFlags::noWindowClose);
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
                call(0x004969DA); // nop this sets the real time not used
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
                        ObjectManager::updateYearly1();
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
            variableUpdate();
        else
            fixedUpdate();
    }

    // 0x00406386
    static void run()
    {
#ifdef _WIN32
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
        sub_4062D1();
        sub_406417(nullptr);

#ifdef _READ_REGISTRY_
        constexpr auto INSTALL_REG_KEY = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{77F45E76-E897-42CA-A9FE-5F56817D875C}";

        HKEY key;
        if (RegOpenKeyA(HKEY_LOCAL_MACHINE, INSTALL_REG_KEY, &key) == ERROR_SUCCESS)
        {
            DWORD type;
            DWORD dataSize = gCDKey.size();
            RegQueryValueExA(key, "CDKey", nullptr, &type, (LPBYTE)gCDKey.get(), &dataSize);
            RegCloseKey(key);
        }
#endif

        // Call tick before Ui::processMessages to ensure initialise is called
        // otherwise window events can end up using an uninitialised window manager.
        // This can be removed when initialise is moved out of tick().
        tick();

        while (Ui::processMessages())
        {
            if (addr<0x005252AC, uint32_t>() != 0)
            {
                sub_4058F5();
            }
            sub_4062E0();
            update();
        }
        sub_40567E();

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

    void simulateGame(const fs::path& path, int32_t ticks)
    {
        Config::read();
        Environment::resolvePaths();
        resetCmdline();
        registerHooks();

        try
        {
            initialise();
            loadFile(path);
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

    namespace unsafe
    {
        template<typename T>
        constexpr auto begin(const T& item)
        {
            return reinterpret_cast<const char*>(&item);
        }
        template<typename T>
        constexpr auto end(const T& item)
        {
            return reinterpret_cast<const char*>(&item) + sizeof(T);
        }
        template<typename T>
        auto bitwise_equal(const T& lhs, const T& rhs)
        {
            return std::equal(begin(lhs), end(lhs), // will become constexpr with C++20
                              begin(rhs),
                              end(rhs));
        }
        template<typename T1, typename T2>
        auto bitwise_equal(const T1& lhs, const T2& rhs)
        {
            return std::equal(begin(lhs), end(lhs), // will become constexpr with C++20
                              begin(rhs),
                              end(rhs));
        }

        template<typename T1, typename T2>
        void bitWiseLogDivergence(const std::string entity, const T1& lhs, const T2& rhs)
        {
            size_t size = sizeof(T1) / sizeof(char);
            char* array_lhs = (char*)(&lhs);
            char* array_rhs = (char*)(&rhs);

            for (size_t offset = 0; offset < size; offset++)
            {
                if (array_lhs[offset] != array_rhs[offset])
                {
                    Logging::info("DIVERGENCE");
                    Logging::info("ENTITY: {{ {} }}", entity);
                    Logging::info("\tOFFSET: {{ {} }}", offset);
                    Logging::info("\t   LHS: {{ {:#x} }}", array_lhs[offset]);
                    Logging::info("\t   RHS: {{ {:#x} }}", array_rhs[offset]);
                }
            }
        }
    }

    template<typename T1, typename T2>
    void logEntityDivergence(const std::string entity, const T1& lhs, const T2& rhs)
    {
        for (int offset = 0; offset < sizeof(lhs); offset++)
            unsafe::bitWiseLogDivergence(entity + "[" + std::to_string(offset) + "]", lhs[offset], rhs[offset]);
    }

    template<typename T1, typename T2>
    void logEntityDivergence(const std::string entity, const T1& lhs, const T2& rhs, int arraySize)
    {
        for (int offset = 0; offset < arraySize; offset++)
            unsafe::bitWiseLogDivergence(entity + "[" + std::to_string(offset) + "]", lhs[offset], rhs[offset]);
    }

    template<typename T>
    void logDivergentGameStateField(const std::string entity, int offset, const T& lhs, const T& rhs)
    {
        if (!unsafe::bitwise_equal(lhs, rhs))
        {
            Logging::info("DIVERGENCE");
            Logging::info("ENTITY: {{ {} }}", entity);
            Logging::info("\tOFFSET: {{ {} }}", offset);
            Logging::info("\t   LHS: {{ {:#x} }}", lhs);
            Logging::info("\t   RHS: {{ {:#x} }}", rhs);
        }
    }

    void compareGameStates(const fs::path& path)
    {
        Logging::info("Comparing reference file {} to current GameState frame", path);
        refFile = std::move(S5::importSave(path));
        S5::GameState& refGameState = refFile.get()->gameState;

        logDivergentGameStateField("rng_0:", 0, getGameState().rng.srand_0(), refGameState.rng[0]);
        logDivergentGameStateField("rng_1:", 0, getGameState().rng.srand_1(), refGameState.rng[1]);

        if (getGameState().flags != refGameState.flags)
        {
            auto flags1 = static_cast<uint32_t>(getGameState().flags);
            auto flags2 = static_cast<uint32_t>(refGameState.flags);
            Logging::info("DIVERGENCE");
            Logging::info("ENTITY: {{ {} }}", "flags");
            Logging::info("\tOFFSET: {{ {} }}", 0);
            Logging::info("\tflags:    LHS: {{ {:#x} }}", flags1);
            Logging::info("\tflags:    LHS: {{ {:#x} }}", flags2);
        }

        logDivergentGameStateField("currentDay:", 0, getGameState().currentDay, refGameState.currentDay);
        logDivergentGameStateField("dayCounter:", 0, getGameState().dayCounter, refGameState.dayCounter);
        logDivergentGameStateField("currentYear:", 0, getGameState().currentYear, refGameState.currentYear);
        logDivergentGameStateField("currentMonth:", 0, getGameState().currentMonth, refGameState.currentMonth);
        logDivergentGameStateField("currentMonthOfMonth:", 0, getGameState().currentDayOfMonth, refGameState.currentDayOfMonth);

        logEntityDivergence("companies", getGameState().companies, refGameState.companies, Limits::kMaxCompanies);
        logEntityDivergence("towns", getGameState().towns, refGameState.towns, Limits::kMaxTowns);
        logEntityDivergence("industries", getGameState().industries, refGameState.industries, Limits::kMaxIndustries);
        logEntityDivergence("stations", getGameState().stations, refGameState.stations, Limits::kMaxStations);
        logEntityDivergence("entities", getGameState().entities, refGameState.entities, Limits::kMaxEntities);
        logEntityDivergence("animations", getGameState().animations, refGameState.animations, Limits::kMaxAnimations);
        logEntityDivergence("waves", getGameState().waves, refGameState.waves, Limits::kMaxWaves);
        logEntityDivergence("userStrings ", getGameState().userStrings, refGameState.userStrings, Limits::kMaxUserStrings);

        for (int route = 0; route < Limits::kMaxVehicles; route++)
            for (int routePerVehicle = 0; routePerVehicle < Limits::kMaxRoutingsPerVehicle; routePerVehicle++)
            {
                logDivergentGameStateField(
                    "routings[" + std::to_string(route) + "][" + std::to_string(routePerVehicle) + "]",
                    route * Limits::kMaxOrdersPerVehicle + routePerVehicle,
                    getGameState().routings[route][routePerVehicle],
                    refGameState.routings[route][routePerVehicle]);
            }

        logEntityDivergence("orders", getGameState().orders, refGameState.orders);
    }

    // 0x00406D13
    static int main(const CommandLineOptions& options)
    {
        // Bootstrap the logging system.
        Logging::initialize(options.logLevels);

        // Always print the product name and version first.
        Logging::info("{}", OpenLoco::getVersionInfo());

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
            Ui::initialiseInput();
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
