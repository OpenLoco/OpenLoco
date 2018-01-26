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
#endif

#include "audio/audio.h"
#include "config.h"
#include "date.h"
#include "environment.h"
#include "graphics/gfx.h"
#include "input.h"
#include "interop/interop.hpp"
#include "intro.h"
#include "localisation/string_ids.h"
#include "objects/objectmgr.h"
#include "openloco.h"
#include "platform/platform.h"
#include "progressbar.h"
#include "scenariomgr.h"
#include "stationmgr.h"
#include "things/thingmgr.h"
#include "tutorial.h"
#include "ui.h"
#include "utility/numeric.hpp"
#include "windowmgr.h"

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

using namespace openloco::interop;
namespace windowmgr = openloco::ui::windowmgr;
using input_flags = openloco::input::input_flags;
using input_state = openloco::input::input_state;
using window_type = openloco::ui::window_type;

namespace openloco
{
    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";

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
    loco_global<uint8_t, 0x0050AF26> byte_50AF26;
    loco_global<uint32_t, 0x00525E18> _srand0;
    loco_global<uint32_t, 0x00525E1C> _srand1;
    loco_global<uint32_t, 0x00525F5E> _scenario_ticks;

    static void tick_logic(int32_t count);
    static void tick_logic();
    static void tick_wait();
    static void date_tick();

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

    bool is_editor_mode()
    {
        return (_screen_flags & screen_flags::editor) != 0;
    }

    bool is_title_mode()
    {
        return (_screen_flags & screen_flags::title) != 0;
    }

    bool is_paused()
    {
        return paused_state;
    }

    uint32_t scenario_ticks()
    {
        return _scenario_ticks;
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
    static void* malloc(size_t size)
    {
        return ((void* (*)(size_t))0x004D1401)(size);
    }

    /**
     * Use this to reallocate memory that will be freed in vanilla code or via loco_free.
     */
    static void* realloc(void* address, size_t size)
    {
        return ((void* (*)(void*, size_t))0x004D1B28)(address, size);
    }

    /**
     * Use this to free up memory allocated in vanilla code or via loco_malloc / loco_realloc.
     */
    static void free(void* address)
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

    static void sub_431A8A(uint16_t bx, uint16_t dx)
    {
        registers regs;
        regs.bx = bx;
        regs.dx = dx;
        call(0x00431A8A, regs);
    }

    // 0x00407FFD
    static bool is_already_running(const char* mutexName)
    {
        auto result = ((int32_t(*)(const char*))(0x00407FFD))(mutexName);
        return result != 0;
    }

    // 0x004BE621
    static void exit_with_error(string_id eax, string_id ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        call(0x004BE621, regs);
    }

    // 0x0044155B
    static void check_game_file_exists(int i)
    {
        registers regs;
        regs.ebx = i;
        call(0x0044155B, regs);
    }

    // 0x0044154B
    static void check_game_files_exist()
    {
        for (int i = 0; i < 48; i++)
        {
            check_game_file_exists(i);
        }
    }

    // 0x004414C5
    static void check_game_files_are_valid()
    {
        call(0x004414C5);
    }

    static void sub_441444()
    {
        call(0x00441444);
    }

    // 0x00441400
    static void startup_checks()
    {
        if (is_already_running("Locomotion"))
        {
            exit_with_error(61, 1016);
        }
        else
        {
            check_game_files_exist();
            check_game_files_are_valid();
            sub_441444();
        }
    }

    // 0x004C57C0
    static void initialise_viewports()
    {
        call(0x004C57C0);
    }

    static void initialise()
    {
        addr<0x0050C18C, int32_t>() = addr<0x00525348, int32_t>();
        call(0x004078BE);
        call(0x004BF476);
        environment::resolve_paths();
        progressbar::begin(string_ids::loading, 0);
        progressbar::set_progress(30);
        startup_checks();
        progressbar::set_progress(40);
        call(0x004BE5DE);
        progressbar::end();
        config::read();
        objectmgr::load_index();
        scenariomgr::load_index(0);
        progressbar::begin(string_ids::loading, 0);
        progressbar::set_progress(60);
        gfx::load_g1();
        progressbar::set_progress(220);
        call(0x004949BC);
        progressbar::set_progress(235);
        progressbar::set_progress(250);
        ui::initialise_cursors();
        progressbar::end();
        ui::initialise();
        audio::initialise();
        initialise_viewports();
        call(0x004284C8);
        call(0x004969DA);
        call(0x0043C88C);
        addr<0x00508F14, int16_t>() |= 0x20;
#ifdef _SHOW_INTRO_
        intro::state(intro::intro_state::begin);
#else
        intro::state(intro::intro_state::end);
#endif
        call(0x0046AD7D);
        call(0x00438A6C);
        gfx::clear(gfx::screen_dpi(), 0x0A0A0A0A);
    }

    // 0x0048AC66
    static void play_title_screen_music()
    {
        call(0x0048AC66);
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
            register_hook(
                0x0046AD71,
                [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                    longjmp(tickJump, 1);
                });

            initialise();
            last_tick_time = platform::get_time();
        }

        uint32_t time = platform::get_time();
        time_since_last_tick = std::min(time - last_tick_time, 500U);
        last_tick_time = time;

        if (!is_paused())
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
        if (byte_50AF26 == 0)
        {
            byte_50AF26 = 16;
            gfx::clear(gfx::screen_dpi(), 0);
            ui::get_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
            addr<0x00F2539C, int32_t>() = 0;
        }
        else
        {
            if (byte_50AF26 >= 16)
            {
                byte_50AF26++;
                if (byte_50AF26 >= 48)
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
                ui::set_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                gfx::invalidate_screen();
                if (byte_50AF26 != 96)
                {
                    tick_wait();
                    return;
                }
                byte_50AF26 = 1;
                if (addr<0x00F2539C, int32_t>() != 0)
                {
                    byte_50AF26 = 2;
                }
                config::write();
            }

            call(0x00452D1A);
            call(0x00440DEC);

            if (addr<0x00525340, int32_t>() == 1)
            {
                addr<0x00525340, int32_t>() = 0;
                addr<0x00508F10, uint16_t>() |= (1 << 1);
            }

            input::handle_keyboard();
            sub_48A18C();

            addr<0x0050C1AE, int32_t>()++;
            if (intro::is_active())
            {
                intro::update();
            }
            else
            {
                uint16_t numUpdates = std::clamp<uint16_t>(time_since_last_tick / (uint16_t)31, 1, 3);
                if (windowmgr::find(window_type::multiplayer, 0) != nullptr)
                {
                    numUpdates = 1;
                }
                if (_screen_flags & screen_flags::unknown_2)
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
                    switch (input::state())
                    {
                        case input_state::reset:
                        case input_state::normal:
                        case input_state::dropdown_active:
                            if (input::has_flag(input_flags::viewport_scrolling))
                            {
                                input::reset_flag(input_flags::viewport_scrolling);
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
                if (is_paused())
                {
                    numUpdates = 0;
                }
                addr<0x00F253A0, uint16_t>() = std::max<uint16_t>(1, numUpdates);
                _screen_age = std::min(0xFFFF, (int32_t)_screen_age + addr<0x00F253A0, int16_t>());
                if (game_speed != 0)
                {
                    numUpdates *= 3;
                    if (game_speed != 1)
                    {
                        numUpdates *= 3;
                    }
                }

                call(0x0046FFCA);
                tick_logic(numUpdates);

                addr<0x00525F62, int16_t>()++;
                call(0x0043D9D4);
                call(0x0048A78D);
                play_title_screen_music();
                if (tutorial::state() != tutorial::tutorial_state::none && addr<0x0052532C, int32_t>() == 0 && addr<0x0113E2E4, int32_t>() < 0x40)
                {
                    tutorial::stop();

                    // This ends with a premature tick termination
                    call(0x0043C0FD);
                    return; // won't be reached
                }

                call(0x00431695);
                call(0x00452B5F);
                call(0x0046FFCA);
                if (addr<0x0050AEC0, uint8_t>() != 0xFF)
                {
                    addr<0x0050AEC0, uint8_t>()++;
                    if (addr<0x0050AEC0, uint8_t>() != 0xFF)
                    {
                        config::write();
                    }
                }
            }

            if (byte_50AF26 == 2)
            {
                addr<0x005252DC, int32_t>() = 1;
                ui::get_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
                ui::set_cursor_pos(addr<0x00F2538C, int32_t>(), addr<0x00F25390, int32_t>());
            }
        }

        tick_wait();
    }

    static void tick_logic(int32_t count)
    {
        for (int32_t i = 0; i < count; i++)
        {
            tick_logic();
        }
    }

    // 0x004612EC
    static void invalidate_map_animations()
    {
        call(0x004612EC);
    }

    // 0x0046ABCB
    static void tick_logic()
    {
        _scenario_ticks++;
        addr<0x00525F64, int32_t>()++;
        addr<0x00525FCC, int32_t>() = _srand0;
        addr<0x00525FD0, int32_t>() = _srand1;
        call(0x004613F0);
        addr<0x00F25374, uint8_t>() = addr<0x009C871C, uint8_t>();
        date_tick();
        call(0x00463ABA);
        call(0x004C56F6);
        call(0x00496B6D);
        call(0x00453234);
        thingmgr::update_vehicles();
        call(0x0046FFCA);
        call(0x0048B1FA);
        thingmgr::update_misc_things();
        call(0x0046FFCA);
        call(0x00430319);
        invalidate_map_animations();
        call(0x0048A73B);
        call(0x0048ACFD);
        call(0x00444387);

        addr<0x009C871C, uint8_t>() = addr<0x00F25374, uint8_t>();
        if (addr<0x0050C197, uint8_t>() != 0)
        {
            uint16_t bx = 0x043A;
            uint16_t dx = addr<0x0050C198, uint16_t>();
            if (addr<0x0050C197, uint8_t>() == 0xFE)
            {
                bx = addr<0x0050C198, uint16_t>();
                dx = 0xFFFF;
            }
            addr<0x0050C197, uint8_t>() = 0;
            sub_431A8A(bx, dx);
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
    static void date_tick()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
        {
            if (update_day_counter())
            {
                stationmgr::sub_48B244();
                call(0x004B94CF);
                call(0x00453487);
                call(0x004284DB);
                call(0x004969DA);
                call(0x00439BA5);

                auto yesterday = calc_date(current_day() - 1);
                auto today = calc_date(current_day());
                set_date(today);
                sub_496A84(today.day_of_olympiad);
                if (today.month != yesterday.month)
                {
                    // End of every month
                    addr<0x0050A004, uint16_t>() += 2;
                    addr<0x00526243, uint16_t>()++;
                    call(0x0049748C);
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
    void tick_wait()
    {
        do
        {
            // Idle loop for a 40 FPS
        } while (platform::get_time() - last_tick_time < 25);
    }

    void prompt_tick_loop(std::function<bool()> tickAction)
    {
        while (true)
        {
            auto startTime = platform::get_time();
            time_since_last_tick = 31;
            if (!ui::process_messages() || !tickAction())
            {
                break;
            }
            ui::render();
            do
            {
                // Idle loop for a 40 FPS
            } while (platform::get_time() - startTime < 25);
        }
    }

    void sub_48A18C()
    {
        call(0x0048A18C);
    }

    uint32_t rand_next()
    {
        auto srand0 = _srand0;
        auto srand1 = _srand1;
        _srand0 = utility::ror<uint32_t>(srand1 ^ 0x1234567F, 7);
        _srand1 = utility::ror<uint32_t>(srand0, 3);
        return _srand1;
    }

    int32_t rand_next(int32_t high)
    {
        return rand_next(0, high);
    }

    int32_t rand_next(int32_t low, int32_t high)
    {
        int32_t positive = rand_next() & 0x7FFFFFFF;
        return low + (positive % ((high + 1) - low));
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

        while (ui::process_messages())
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
        std::cout << "OpenLoco v0.1" << std::endl;
        try
        {
            register_hooks();
            if (sub_4054B9())
            {
                ui::create_window();
                call(0x004078FE);
                call(0x00407B26);
                ui::initialise_input();
                audio::initialise_dsound();
                run();
                audio::dispose_dsound();
                ui::dispose_cursors();
                ui::dispose_input();

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
    openloco::glpCmdLine = lpCmdLine;
    openloco::ghInstance = hInstance;
    openloco::main();
    return 0;
}
#endif
}
