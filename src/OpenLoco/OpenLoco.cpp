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
#include "CompanyManager.h"
#include "Config.h"
#include "Console.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "EditorController.h"
#include "Entities/EntityManager.h"
#include "Entities/EntityTweener.h"
#include "Environment.h"
#include "Game.h"
#include "GameException.hpp"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Gui.h"
#include "IndustryManager.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "Intro.h"
#include "Localisation/LanguageFiles.h"
#include "Localisation/Languages.h"
#include "Localisation/StringIds.h"
#include "MultiPlayer.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Platform/Crash.h"
#include "Platform/Platform.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "ScenarioManager.h"
#include "StationManager.h"
#include "Title.h"
#include "TownManager.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "Ui/WindowManager.h"
#include "Utility/Numeric.hpp"
#include "ViewportManager.h"

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Input;

namespace OpenLoco
{
    using Clock = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;

    static double _accumulator = 0.0;
    static Timepoint _lastUpdate = Clock::now();

#ifdef _WIN32
    loco_global<HINSTANCE, 0x0113E0B4> ghInstance;
    loco_global<LPSTR, 0x00525348> glpCmdLine;
#else
    loco_global<char*, 0x00525348> glpCmdLine;
#endif

    loco_global<char[256], 0x005060D0> gCDKey;

    loco_global<uint16_t, 0x0050C19C> time_since_last_tick;
    loco_global<uint32_t, 0x0050C19E> last_tick_time;
    loco_global<uint8_t, 0x00508F08> game_command_nest_level;
    loco_global<uint16_t, 0x00508F12> _screen_age;
    loco_global<uint16_t, 0x00508F14> _screenFlags;
    loco_global<uint8_t, 0x00508F17> paused_state;
    loco_global<uint8_t, 0x00508F1A> _gameSpeed;
    static loco_global<string_id, 0x0050A018> _mapTooltipFormatArguments;
    static loco_global<int32_t, 0x0052339C> _52339C;
    static loco_global<int8_t, 0x0052336E> _52336E; // bool
    loco_global<Utility::prng, 0x00525E18> _prng;
    static loco_global<CompanyId_t[2], 0x00525E3C> _playerCompanies;
    loco_global<uint32_t, 0x00525F5E> _scenario_ticks;
    static loco_global<int16_t, 0x00525F62> _525F62;

    static loco_global<CompanyId_t, 0x009C68EB> _updating_company_id;

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

#ifdef _WIN32
    void* hInstance()
    {
        return ghInstance;
    }
#endif

    const char* lpCmdLine()
    {
        return glpCmdLine;
    }

#ifndef _WIN32
    void lpCmdLine(const char* path)
    {
        glpCmdLine = strdup(path);
    }
#endif

    void resetScreenAge()
    {
        _screen_age = 0;
    }

    uint16_t getScreenAge()
    {
        return _screen_age;
    }

    uint16_t getScreenFlags()
    {
        return _screenFlags;
    }

    void setAllScreenFlags(uint16_t newScreenFlags)
    {
        _screenFlags = newScreenFlags;
    }

    void setScreenFlag(uint16_t value)
    {
        *_screenFlags |= value;
    }

    void clearScreenFlag(uint16_t value)
    {
        *_screenFlags &= ~value;
    }

    bool isEditorMode()
    {
        return (getScreenFlags() & ScreenFlags::editor) != 0;
    }

    bool isTitleMode()
    {
        return (getScreenFlags() & ScreenFlags::title) != 0;
    }

    bool isNetworked()
    {
        return (getScreenFlags() & ScreenFlags::networked) != 0;
    }

    bool isNetworkHost()
    {
        return (getScreenFlags() & ScreenFlags::networkHost) != 0;
    }

    bool isProgressBarActive()
    {
        return (getScreenFlags() & ScreenFlags::progressBarActive) != 0;
    }

    bool isInitialised()
    {
        return (getScreenFlags() & ScreenFlags::initialised) != 0;
    }

    bool isDriverCheatEnabled()
    {
        return (getScreenFlags() & ScreenFlags::driverCheatEnabled) != 0;
    }

    bool isSandboxMode()
    {
        return (getScreenFlags() & ScreenFlags::sandboxMode) != 0;
    }

    bool isPauseOverrideEnabled()
    {
        return (getScreenFlags() & ScreenFlags::pauseOverrideEnabled) != 0;
    }

    bool isPaused()
    {
        return paused_state != 0;
    }

    uint8_t getPauseFlags()
    {
        return paused_state;
    }

    void setPauseFlag(uint8_t value)
    {
        *paused_state |= value;
    }

    void unsetPauseFlag(uint8_t value)
    {
        *paused_state &= ~(value);
    }

    uint8_t getGameSpeed()
    {
        return _gameSpeed;
    }

    void setGameSpeed(uint8_t speed)
    {
        assert(speed >= 0 && speed <= 3);
        _gameSpeed = speed;
    }

    uint32_t scenarioTicks()
    {
        return _scenario_ticks;
    }

    Utility::prng& gPrng()
    {
        return _prng;
    }

    static bool sub_4054B9()
    {
        registers regs;
        call(0x004054B9, regs);
        return regs.eax != 0;
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
        call(0x004062D1);
    }

    static void sub_406417()
    {
        ((void (*)())0x00406417)();
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
        call(0x004062E0);
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
    static void exitWithError(string_id eax, string_id ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        call(0x004BE621, regs);
    }

    // 0x004BE5EB
    void exitWithError(string_id message, uint32_t errorCode)
    {
        // Saves the error code for later writing to error log 1.TMP.
        registers regs;
        regs.eax = errorCode;
        regs.bx = message;
        call(0x004BE5EB, regs);
    }

    // 0x004BE65E
    [[noreturn]] void exitCleanly()
    {
        Audio::disposeDSound();
        Audio::close();
        Ui::disposeCursors();
        Ui::disposeInput();
        Localisation::unloadLanguageFile();

        auto tempFilePath = Environment::getPathNoWarning(Environment::path_id::_1tmp);
        if (fs::exists(tempFilePath))
        {
            auto path8 = tempFilePath.u8string();
            printf("Removing temp file '%s'\n", path8.c_str());
            fs::remove(tempFilePath);
        }

        // SDL_Quit();
        exit(0);
    }

    // 0x00441400
    static void startupChecks()
    {
        if (isAlreadyRunning("Locomotion"))
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

        Colour::initColourMap();
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
        std::srand(std::time(0));
        addr<0x0050C18C, int32_t>() = addr<0x00525348, int32_t>();
        call(0x004078BE);
        call(0x004BF476);
        Environment::resolvePaths();
        Localisation::enumerateLanguages();
        Localisation::loadLanguageFile();
        Ui::ProgressBar::begin(StringIds::loading);
        Ui::ProgressBar::setProgress(30);
        startupChecks();
        Ui::ProgressBar::setProgress(40);
        call(0x004BE5DE);
        Ui::ProgressBar::end();
        Config::read();
        ObjectManager::loadIndex();
        ScenarioManager::loadIndex(0);
        Ui::ProgressBar::begin(StringIds::loading);
        Ui::ProgressBar::setProgress(60);
        Gfx::loadG1();
        Ui::ProgressBar::setProgress(220);
        call(0x004949BC);
        Ui::ProgressBar::setProgress(235);
        Ui::ProgressBar::setProgress(250);
        Ui::initialiseCursors();
        Ui::ProgressBar::end();
        Ui::initialise();
        initialiseViewports();
        call(0x004284C8);
        call(0x004969DA);
        call(0x0043C88C);
        setScreenFlag(ScreenFlags::initialised);
#ifdef _SHOW_INTRO_
        Intro::state(Intro::State::begin);
#else
        Intro::state(Intro::State::end);
#endif
        Title::start();
        Gui::init();
        Gfx::clear(Gfx::screenContext(), 0x0A0A0A0A);
    }

    // 0x00428E47
    static void sub_428E47()
    {
        call(0x00428E47);
    }

    // 0x00444387
    void sub_444387()
    {
        call(0x00444387);
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
        if (!isNetworked())
        {
            _updating_company_id = CompanyManager::getControllingId();
            for (auto i = 0; i < var_F253A0; i++)
            {
                sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            Ui::handleInput();
            CompanyManager::updateOwnerStatus();
            return;
        }

        // Only run every other tick?
        if (_525F62 % 2 != 0)
        {
            return;
        }

        // Host/client?
        if (isNetworkHost())
        {
            _updating_company_id = CompanyManager::getControllingId();

            // run twice as often as var_F253A0
            for (auto i = 0; i < var_F253A0 * 2; i++)
            {
                sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            WindowManager::update();
            Ui::handleInput();
            CompanyManager::updateOwnerStatus();
            sub_46E388();

            _updating_company_id = _playerCompanies[1];
            sub_4317BD();
        }
        else
        {
            _updating_company_id = _playerCompanies[1];
            auto eax = sub_4317BD();

            _updating_company_id = _playerCompanies[0];
            if (!isTitleMode())
            {
                auto edx = _prng->srand_0();
                edx ^= CompanyManager::get(0)->cash.var_00;
                edx ^= CompanyManager::get(1)->cash.var_00;
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
                sub_428E47();
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
        Console::log("Tick interrupted");
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
        static loco_global<void*, 0x0050C1A6> tickJumpESP;
        static uint8_t spareStackMemory[2048];
        tickJumpESP = spareStackMemory + sizeof(spareStackMemory);

        if (setjmp(tickJump))
        {
            // Premature end of current tick
            tickInterrupted();
            return;
        }

        try
        {
            addr<0x00113E87C, int32_t>() = 0;
            addr<0x0005252E0, int32_t>() = 0;
            if (!isInitialised)
            {
                isInitialised = true;

                // This address is where those routines jump back to to end the tick prematurely
                registerHook(
                    0x0046AD71,
                    [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                        longjmp(tickJump, 1);
                    });

                initialise();
                last_tick_time = platform::getTime();
            }

            uint32_t time = platform::getTime();
            time_since_last_tick = (uint16_t)std::min(time - last_tick_time, 500U);
            last_tick_time = time;

            if (!isPaused())
            {
                addr<0x0050C1A2, uint32_t>() += time_since_last_tick;
            }
            if (Tutorial::state() != Tutorial::State::none)
            {
                time_since_last_tick = 31;
            }
            game_command_nest_level = 0;
            Ui::update();

            addr<0x005233AE, int32_t>() += addr<0x0114084C, int32_t>();
            addr<0x005233B2, int32_t>() += addr<0x01140840, int32_t>();
            addr<0x0114084C, int32_t>() = 0;
            addr<0x01140840, int32_t>() = 0;
            if (Config::get().var_72 == 0)
            {
                Config::get().var_72 = 16;
                Ui::getCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                Gfx::clear(Gfx::screenContext(), 0);
                addr<0x00F2539C, int32_t>() = 0;
            }
            else
            {
                if (Config::get().var_72 >= 16)
                {
                    Config::get().var_72++;
                    if (Config::get().var_72 >= 48)
                    {
                        if (sub_4034FC(addr<0x00F25394, int32_t>(), addr<0x00F25398, int32_t>()))
                        {
                            uintptr_t esi = addr<0x00F25390, int32_t>() + 4;
                            esi *= addr<0x00F25398, int32_t>();
                            esi += addr<0x00F2538C, int32_t>();
                            esi += 2;
                            esi += addr<0x00F25394, int32_t>();
                            addr<0x00F2539C, int32_t>() |= *((int32_t*)esi);
                            call(0x00403575);
                        }
                    }
                    Ui::setCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                    Gfx::invalidateScreen();
                    if (Config::get().var_72 != 96)
                    {
                        return;
                    }
                    Config::get().var_72 = 1;
                    if (addr<0x00F2539C, int32_t>() != 0)
                    {
                        Config::get().var_72 = 2;
                    }
                    Config::write();
                }

                call(0x00452D1A);
                call(0x00440DEC);

                if (addr<0x00525340, int32_t>() == 1)
                {
                    addr<0x00525340, int32_t>() = 0;
                    MultiPlayer::setFlag(MultiPlayer::flags::flag_1);
                }

                Input::handleKeyboard();
                Audio::updateSounds();

                addr<0x0050C1AE, int32_t>()++;
                if (Intro::isActive())
                {
                    Intro::update();
                }
                else
                {
                    uint16_t numUpdates = std::clamp<uint16_t>(time_since_last_tick / (uint16_t)31, 1, 3);
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
                    addr<0x0052622E, int16_t>() += numUpdates;
                    if (isPaused())
                    {
                        numUpdates = 0;
                    }
                    uint16_t var_F253A0 = std::max<uint16_t>(1, numUpdates);
                    _screen_age = std::min(0xFFFF, (int32_t)_screen_age + var_F253A0);
                    if (_gameSpeed != 0)
                    {
                        numUpdates *= 3;
                        if (_gameSpeed != 1)
                        {
                            numUpdates *= 3;
                        }
                    }

                    sub_46FFCA();
                    tickLogic(numUpdates);

                    _525F62++;
                    if (isEditorMode())
                    {
                        EditorController::tick();
                    }
                    Audio::playBackgroundMusic();

                    // TODO move stop title music to title::stop (when mode changes)
                    if (!isTitleMode())
                    {
                        Audio::stopTitleMusic();
                    }

                    if (Tutorial::state() != Tutorial::State::none && addr<0x0052532C, int32_t>() != 0 && addr<0x0113E2E4, int32_t>() < 0x40)
                    {
                        Tutorial::stop();

                        // This ends with a premature tick termination
                        Game::returnToTitle();
                        return; // won't be reached
                    }

                    sub_431695(var_F253A0);
                    call(0x00452B5F);
                    sub_46FFCA();
                    if (Config::get().countdown != 0xFF)
                    {
                        Config::get().countdown++;
                        if (Config::get().countdown != 0xFF)
                        {
                            Config::write();
                        }
                    }
                }

                if (Config::get().var_72 == 2)
                {
                    addr<0x005252DC, int32_t>() = 1;
                    Ui::getCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                    Ui::setCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
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

    // 0x004612EC
    static void invalidate_map_animations()
    {
        call(0x004612EC);
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

    static loco_global<int8_t, 0x0050C197> _50C197;
    static loco_global<string_id, 0x0050C198> _50C198;

    // 0x0046ABCB
    static void tickLogic()
    {
        _scenario_ticks++;
        addr<0x00525F64, int32_t>()++;
        addr<0x00525FCC, uint32_t>() = _prng->srand_0();
        addr<0x00525FD0, uint32_t>() = _prng->srand_1();
        call(0x004613F0);
        addr<0x00F25374, uint8_t>() = S5::getOptions().madeAnyChanges;
        dateTick();
        call(0x00463ABA);
        call(0x004C56F6);
        TownManager::update();
        IndustryManager::update();
        EntityManager::updateVehicles();
        sub_46FFCA();
        StationManager::update();
        EntityManager::updateMiscEntities();
        sub_46FFCA();
        CompanyManager::update();
        invalidate_map_animations();
        Audio::updateVehicleNoise();
        Audio::updateAmbientNoise();
        Title::update();

        S5::getOptions().madeAnyChanges = addr<0x00F25374, uint8_t>();
        if (_50C197 != 0)
        {
            auto title = StringIds::error_unable_to_load_saved_game;
            auto message = _50C198;
            if (_50C197 == -2)
            {
                title = _50C198;
                message = StringIds::null;
            }
            _50C197 = 0;
            Ui::Windows::showError(title, message);
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
            auto autosaveDirectory = Environment::getPath(Environment::path_id::autosave);
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

                auto amountToKeep = static_cast<size_t>(std::max(1, Config::getNew().autosave_amount));
                if (autosaveFiles.size() > amountToKeep)
                {
                    // Sort them by name (which should correspond to date order)
                    std::sort(autosaveFiles.begin(), autosaveFiles.end());

                    // Delete excess files
                    auto numToDelete = autosaveFiles.size() - amountToKeep;
                    for (size_t i = 0; i < numToDelete; i++)
                    {
                        auto path8 = autosaveFiles[i].u8string();
                        std::printf("Deleting old autosave: %s\n", path8.c_str());
                        fs::remove(autosaveFiles[i]);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to clean autosaves: %s\n", e.what());
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
            auto autosaveDirectory = Environment::getPath(Environment::path_id::autosave);
            Environment::autoCreateDirectory(autosaveDirectory);

            auto autosaveFullPath = autosaveDirectory / filename;

            auto autosaveFullPath8 = autosaveFullPath.u8string();
            std::printf("Autosaving game to %s\n", autosaveFullPath8.c_str());
            S5::save(autosaveFullPath, static_cast<S5::SaveFlags>(S5::SaveFlags::noWindowClose));
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to autosave game: %s\n", e.what());
        }
    }

    static void autosaveCheck()
    {
        _monthsSinceLastAutosave++;

        if (!isTitleMode())
        {
            auto freq = Config::getNew().autosave_frequency;
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
        if ((addr<0x00525E28, uint32_t>() & 1) && !isEditorMode())
        {
            if (updateDayCounter())
            {
                StationManager::updateDaily();
                call(0x004B94CF);
                call(0x00453487);
                call(0x004284DB);
                call(0x004969DA);
                call(0x00439BA5);

                auto yesterday = calcDate(getCurrentDay() - 1);
                auto today = calcDate(getCurrentDay());
                setDate(today);
                Scenario::updateSnowLine(today.day_of_olympiad);
                if (today.month != yesterday.month)
                {
                    // End of every month
                    Ui::Windows::TimePanel::invalidateFrame();
                    addr<0x00526243, uint16_t>()++;
                    TownManager::updateMonthly();
                    call(0x0045383B);
                    call(0x0043037B);
                    call(0x0042F213);
                    call(0x004C3C54);

                    if (today.year <= 2029)
                    {
                        Economy::updateMonthly();
                    }

                    // clang-format off
                    if (today.month == month_id::january ||
                        today.month == month_id::april ||
                        today.month == month_id::july ||
                        today.month == month_id::october)
                    // clang-format on
                    {
                        CompanyManager::updateQuarterly();
                    }

                    if (today.year != yesterday.year)
                    {
                        // End of every year
                        call(0x004312C7);
                        call(0x004796A9);
                        call(0x004C3A9E);
                        call(0x0047AB9B);
                    }

                    autosaveCheck();
                }

                call(0x00437FB8);
            }
        }
    }

    static void tickWait()
    {
        // Idle loop for a 40 FPS
        do
        {
            std::this_thread::yield();
        } while (platform::getTime() - last_tick_time < 25);
    }

    void promptTickLoop(std::function<bool()> tickAction)
    {
        while (true)
        {
            last_tick_time = platform::getTime();
            time_since_last_tick = 31;
            if (!Ui::processMessages() || !tickAction())
            {
                break;
            }
            Ui::render();
            tickWait();
        }
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
            auto timeMissing = static_cast<uint32_t>((UpdateTime - _accumulator) * 1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(timeMissing));
        }

        tick();
        _accumulator -= UpdateTime;

        Ui::render();
    }

    static void update()
    {
        auto timeNow = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(timeNow - _lastUpdate).count() / 1'000'000.0;

        elapsed *= TimeScale;

        _accumulator = std::min(_accumulator + elapsed, MaxUpdateTime);
        _lastUpdate = timeNow;

        if (Config::getNew().uncapFPS)
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
        sub_406417();

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

    // 0x00406D13
    void main()
    {
        crash_init();
        auto versionInfo = OpenLoco::getVersionInfo();
        std::cout << versionInfo << std::endl;
        try
        {
            const auto& cfg = Config::readNewConfig();
            Environment::resolvePaths();

            registerHooks();
            if (sub_4054B9())
            {
                Ui::createWindow(cfg.display);
                call(0x004078FE);
                call(0x00407B26);
                Ui::initialiseInput();
                Audio::initialiseDSound();
                run();
                exitCleanly();

                // TODO extra clean up code
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
        }
    }
}

extern "C" {

#ifdef _WIN32
/**
     * The function that is called directly from the host application (loco.exe)'s WinMain. This will be removed when OpenLoco can
     * be built as a stand alone application.
     */
// Hack to trick mingw into thinking we forward-declared this function.
__declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
__declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    OpenLoco::glpCmdLine = lpCmdLine;
    OpenLoco::ghInstance = hInstance;
    OpenLoco::main();
    return 0;
}
#endif
}
