#include <algorithm>
#include <cstring>
#include <iostream>
#include <setjmp.h>
#include <string>
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
#include "Date.h"
#include "Environment.h"
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
#include "Platform/Platform.h"
#include "ProgressBar.h"
#include "S5/S5.h"
#include "ScenarioManager.h"
#include "StationManager.h"
#include "Things/ThingManager.h"
#include "Title.h"
#include "TownManager.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "Utility/Numeric.hpp"
#include "ViewportManager.h"

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

using namespace OpenLoco::Interop;
using namespace OpenLoco::ui;
using namespace OpenLoco::Input;

namespace OpenLoco
{
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
    loco_global<uint8_t, 0x00508F14> _screen_flags;
    loco_global<uint8_t, 0x00508F17> paused_state;
    loco_global<uint8_t, 0x00508F1A> game_speed;
    static loco_global<string_id, 0x0050A018> _mapTooltipFormatArguments;
    static loco_global<company_id_t, 0x0050A040> _mapTooltipOwner;
    static loco_global<int32_t, 0x0052339C> _52339C;
    static loco_global<int8_t, 0x0052336E> _52336E; // bool
    loco_global<Utility::prng, 0x00525E18> _prng;
    static loco_global<company_id_t[2], 0x00525E3C> _player_company;
    loco_global<uint32_t, 0x00525F5E> _scenario_ticks;
    static loco_global<int16_t, 0x00525F62> _525F62;

    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;

    static loco_global<uint8_t, 0x009C8714> _editorStep;

    static loco_global<char[256], 0x011367A0> _11367A0;
    static loco_global<char[256], 0x011368A0> _11368A0;

    static void tickLogic(int32_t count);
    static void tickLogic();
    static void tickWait();
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

    uint16_t getScreenAge()
    {
        return _screen_age;
    }

    uint8_t getScreenFlags()
    {
        return _screen_flags;
    }

    bool isEditorMode()
    {
        return (_screen_flags & screen_flags::editor) != 0;
    }

    bool isTitleMode()
    {
        return (_screen_flags & screen_flags::title) != 0;
    }

    bool isNetworked()
    {
        return (_screen_flags & screen_flags::networked) != 0;
    }

    bool isTrackUpgradeMode()
    {
        return (_screen_flags & screen_flags::trackUpgrade) != 0;
    }

    bool isUnknown4Mode()
    {
        return (_screen_flags & screen_flags::unknown_4) != 0;
    }

    bool isUnknown5Mode()
    {
        return (_screen_flags & screen_flags::unknown_5) != 0;
    }

    bool isPaused()
    {
        return paused_state;
    }

    uint8_t getPauseFlags()
    {
        return paused_state;
    }

    // 0x00431E32
    // value: bl (false will no-op)
    // **Gamecommand**
    void togglePause(bool value)
    {
        registers regs;
        regs.bl = value ? 1 : 0;
        call(0x00431E32, regs);
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

    // 0x00441400
    static void startupChecks()
    {
        if (isAlreadyRunning("Locomotion"))
        {
            exitWithError(string_ids::game_init_failure, string_ids::loco_already_running);
        }

        // Originally the game would check that all the game
        // files exist are some have the correct checksum. We
        // do not need to do this anymore, the game should work
        // with g1 alone and some objects?
    }

    // 0x004C57C0
    void initialiseViewports()
    {
        _mapTooltipFormatArguments = string_ids::null;
        _mapTooltipOwner = company_id::null;

        Colour::initColourMap();
        ui::WindowManager::init();
        ui::viewportmgr::init();

        Input::init();
        Input::initMouse();

        // rain-related
        _52339C = -1;

        // tooltip-related
        _52336E = 0;

        ui::textinput::cancel();

        stringmgr::formatString(_11367A0, string_ids::label_button_ok);
        stringmgr::formatString(_11368A0, string_ids::label_button_cancel);
    }

    static void initialise()
    {
        std::srand(std::time(0));
        addr<0x0050C18C, int32_t>() = addr<0x00525348, int32_t>();
        call(0x004078BE);
        call(0x004BF476);
        environment::resolvePaths();
        localisation::enumerateLanguages();
        localisation::loadLanguageFile();
        progressbar::begin(string_ids::loading, 0);
        progressbar::setProgress(30);
        startupChecks();
        progressbar::setProgress(40);
        call(0x004BE5DE);
        progressbar::end();
        config::read();
        objectmgr::loadIndex();
        scenariomgr::loadIndex(0);
        progressbar::begin(string_ids::loading, 0);
        progressbar::setProgress(60);
        Gfx::loadG1();
        progressbar::setProgress(220);
        call(0x004949BC);
        progressbar::setProgress(235);
        progressbar::setProgress(250);
        ui::initialiseCursors();
        progressbar::end();
        ui::initialise();
        initialiseViewports();
        call(0x004284C8);
        call(0x004969DA);
        call(0x0043C88C);
        _screen_flags = _screen_flags | screen_flags::unknown_5;
#ifdef _SHOW_INTRO_
        intro::state(intro::intro_state::begin);
#else
        intro::state(intro::intro_state::end);
#endif
        title::start();
        gui::init();
        Gfx::clear(Gfx::screenDpi(), 0x0A0A0A0A);
    }

    // 0x00428E47
    static void sub_428E47()
    {
        call(0x00428E47);
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
            _updating_company_id = companymgr::getControllingId();
            for (auto i = 0; i < var_F253A0; i++)
            {
                sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            ui::handleInput();
            companymgr::updateOwnerStatus();
            return;
        }

        // Only run every other tick?
        if (_525F62 % 2 != 0)
        {
            return;
        }

        // Host/client?
        if (isTrackUpgradeMode())
        {
            _updating_company_id = companymgr::getControllingId();

            // run twice as often as var_F253A0
            for (auto i = 0; i < var_F253A0 * 2; i++)
            {
                sub_428E47();
                WindowManager::dispatchUpdateAll();
            }

            Input::processKeyboardInput();
            WindowManager::update();
            WindowManager::update();
            ui::handleInput();
            companymgr::updateOwnerStatus();
            sub_46E388();

            _updating_company_id = _player_company[1];
            sub_4317BD();
        }
        else
        {
            _updating_company_id = _player_company[1];
            auto eax = sub_4317BD();

            _updating_company_id = _player_company[0];
            if (!isTitleMode())
            {
                auto edx = _prng->srand_0();
                edx ^= companymgr::get(0)->cash.var_00;
                edx ^= companymgr::get(1)->cash.var_00;
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
            ui::handleInput();
            companymgr::updateOwnerStatus();
            sub_46E388();
        }
    }

    // 0x0043D9D4
    static void editorTick()
    {
        if (!isEditorMode())
            return;

        switch (_editorStep)
        {
            case 0:
                if (WindowManager::find(WindowType::objectSelection) == nullptr)
                    windows::ObjectSelectionWindow::open();
                break;

            case 1:
                // Scenario/landscape loaded?
                if ((addr<0x00525E28, uint32_t>() & 1) != 0)
                    return;

                if (WindowManager::find(WindowType::landscapeGeneration) == nullptr)
                    windows::LandscapeGeneration::open();
                break;

            case 2:
                if (WindowManager::find(WindowType::scenarioOptions) == nullptr)
                    windows::ScenarioOptions::open();
                break;

            case 3:
                break;
        }
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
            std::cout << "tick prematurely ended" << std::endl;
            return;
        }

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
        if (tutorial::state() != tutorial::tutorial_state::none)
        {
            time_since_last_tick = 31;
        }
        game_command_nest_level = 0;
        ui::update();

        addr<0x005233AE, int32_t>() += addr<0x0114084C, int32_t>();
        addr<0x005233B2, int32_t>() += addr<0x01140840, int32_t>();
        addr<0x0114084C, int32_t>() = 0;
        addr<0x01140840, int32_t>() = 0;
        if (config::get().var_72 == 0)
        {
            config::get().var_72 = 16;
            ui::getCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
            Gfx::clear(Gfx::screenDpi(), 0);
            addr<0x00F2539C, int32_t>() = 0;
        }
        else
        {
            if (config::get().var_72 >= 16)
            {
                config::get().var_72++;
                if (config::get().var_72 >= 48)
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
                ui::setCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                Gfx::invalidateScreen();
                if (config::get().var_72 != 96)
                {
                    tickWait();
                    return;
                }
                config::get().var_72 = 1;
                if (addr<0x00F2539C, int32_t>() != 0)
                {
                    config::get().var_72 = 2;
                }
                config::write();
            }

            call(0x00452D1A);
            call(0x00440DEC);

            if (addr<0x00525340, int32_t>() == 1)
            {
                addr<0x00525340, int32_t>() = 0;
                multiplayer::setFlag(multiplayer::flags::flag_1);
            }

            Input::handleKeyboard();
            Audio::updateSounds();

            addr<0x0050C1AE, int32_t>()++;
            if (intro::isActive())
            {
                intro::update();
            }
            else
            {
                uint16_t numUpdates = std::clamp<uint16_t>(time_since_last_tick / (uint16_t)31, 1, 3);
                if (WindowManager::find(ui::WindowType::multiplayer, 0) != nullptr)
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
                        case input_state::reset:
                        case input_state::normal:
                        case input_state::dropdown_active:
                            if (Input::hasFlag(input_flags::viewport_scrolling))
                            {
                                Input::resetFlag(input_flags::viewport_scrolling);
                                numUpdates = 1;
                            }
                            break;
                        case input_state::widget_pressed: break;
                        case input_state::positioning_window: break;
                        case input_state::viewport_right: break;
                        case input_state::viewport_left: break;
                        case input_state::scroll_left: break;
                        case input_state::resizing: break;
                        case input_state::scroll_right: break;
                    }
                }
                addr<0x0052622E, int16_t>() += numUpdates;
                if (isPaused())
                {
                    numUpdates = 0;
                }
                uint16_t var_F253A0 = std::max<uint16_t>(1, numUpdates);
                _screen_age = std::min(0xFFFF, (int32_t)_screen_age + var_F253A0);
                if (game_speed != 0)
                {
                    numUpdates *= 3;
                    if (game_speed != 1)
                    {
                        numUpdates *= 3;
                    }
                }

                sub_46FFCA();
                tickLogic(numUpdates);

                _525F62++;
                editorTick();
                Audio::playBackgroundMusic();

                // TODO move stop title music to title::stop (when mode changes)
                if (!isTitleMode())
                {
                    Audio::stopTitleMusic();
                }

                if (tutorial::state() != tutorial::tutorial_state::none && addr<0x0052532C, int32_t>() != 0 && addr<0x0113E2E4, int32_t>() < 0x40)
                {
                    tutorial::stop();

                    // This ends with a premature tick termination
                    call(0x0043C0FD);
                    return; // won't be reached
                }

                sub_431695(var_F253A0);
                call(0x00452B5F);
                sub_46FFCA();
                if (config::get().countdown != 0xFF)
                {
                    config::get().countdown++;
                    if (config::get().countdown != 0xFF)
                    {
                        config::write();
                    }
                }
            }

            if (config::get().var_72 == 2)
            {
                addr<0x005252DC, int32_t>() = 1;
                ui::getCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                ui::setCursorPos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
            }
        }

        tickWait();
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
        addr<0x00F25374, uint8_t>() = s5::getOptions().madeAnyChanges;
        dateTick();
        call(0x00463ABA);
        call(0x004C56F6);
        townmgr::update();
        industrymgr::update();
        thingmgr::updateVehicles();
        sub_46FFCA();
        stationmgr::update();
        thingmgr::updateMiscThings();
        sub_46FFCA();
        companymgr::update();
        invalidate_map_animations();
        Audio::updateVehicleNoise();
        Audio::updateAmbientNoise();
        call(0x00444387);

        s5::getOptions().madeAnyChanges = addr<0x00F25374, uint8_t>();
        if (_50C197 != 0)
        {
            auto title = string_ids::error_unable_to_load_saved_game;
            auto message = _50C198;
            if (_50C197 == -2)
            {
                title = _50C198;
                message = string_ids::null;
            }
            _50C197 = 0;
            ui::windows::showError(title, message);
        }
    }

    static void sub_496A84(int32_t edx)
    {
        // This is responsible for updating the snow line
        registers regs;
        regs.edx = edx;
        call(0x00496A84, regs);
    }

    // 0x004968C7
    static void dateTick()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !isEditorMode())
        {
            if (updateDayCounter())
            {
                stationmgr::updateDaily();
                call(0x004B94CF);
                call(0x00453487);
                call(0x004284DB);
                call(0x004969DA);
                call(0x00439BA5);

                auto yesterday = calcDate(getCurrentDay() - 1);
                auto today = calcDate(getCurrentDay());
                setDate(today);
                sub_496A84(today.day_of_olympiad);
                if (today.month != yesterday.month)
                {
                    // End of every month
                    addr<0x0050A004, uint16_t>() += 2;
                    addr<0x00526243, uint16_t>()++;
                    townmgr::updateMonthly();
                    call(0x0045383B);
                    call(0x0043037B);
                    call(0x0042F213);
                    call(0x004C3C54);

                    if (today.year <= 2029)
                    {
                        call(0x0046E239);
                    }

                    // clang-format off
                    if (today.month == month_id::january ||
                        today.month == month_id::april ||
                        today.month == month_id::july ||
                        today.month == month_id::october)
                    // clang-format on
                    {
                        // Start of every season?
                        call(0x00487FC1);
                    }

                    if (today.year != yesterday.year)
                    {
                        // End of every year
                        call(0x004312C7);
                        call(0x004796A9);
                        call(0x004C3A9E);
                        call(0x0047AB9B);
                    }
                }

                call(0x00437FB8);
            }
        }
    }

    // 0x0046AD4D
    void tickWait()
    {
        do
        {
            // Idle loop for a 40 FPS
        } while (platform::getTime() - last_tick_time < 25);
    }

    void promptTickLoop(std::function<bool()> tickAction)
    {
        while (true)
        {
            auto startTime = platform::getTime();
            time_since_last_tick = 31;
            if (!ui::processMessages() || !tickAction())
            {
                break;
            }
            ui::render();
            do
            {
                // Idle loop for a 40 FPS
            } while (platform::getTime() - startTime < 25);
        }
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

        // Call tick before ui::processMessages to ensure initialise is called
        // otherwise window events can end up using an uninitialised window manager.
        // This can be removed when initialise is moved out of tick().
        tick();

        while (ui::processMessages())
        {
            if (addr<0x005252AC, uint32_t>() != 0)
            {
                sub_4058F5();
            }
            sub_4062E0();
            tick();
            ui::render();
        }
        sub_40567E();

#ifdef _WIN32
        CoUninitialize();
#endif
    }

    // 0x00406D13
    void main()
    {
        auto versionInfo = OpenLoco::getVersionInfo();
        std::cout << versionInfo << std::endl;
        try
        {
            const auto& cfg = config::readNewConfig();
            environment::resolvePaths();

            registerHooks();
            if (sub_4054B9())
            {
                ui::createWindow(cfg.display);
                call(0x004078FE);
                call(0x00407B26);
                ui::initialiseInput();
                Audio::initialiseDSound();
                run();
                Audio::disposeDSound();
                ui::disposeCursors();
                ui::disposeInput();

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
