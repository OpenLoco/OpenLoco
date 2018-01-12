#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <setjmp.h>

// timeGetTime is unavailable if we use lean and mean
// #define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>

#include "audio/audio.h"
#include "config.h"
#include "graphics/gfx.h"
#include "input.h"
#include "interop/interop.hpp"
#include "intro.h"
#include "objects/objectmgr.h"
#include "openloco.h"
#include "progressbar.h"
#include "scenariomgr.h"
#include "things/thingmgr.h"
#include "tutorial.h"
#include "ui.h"
#include "windowmgr.h"

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

namespace windowmgr = openloco::ui::windowmgr;
using input_flags = openloco::input::input_flags;
using input_state = openloco::input::input_state;
using window_type = openloco::ui::window_type;

namespace openloco
{
    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";

    loco_global<HINSTANCE, 0x0113E0B4> ghInstance;
    loco_global<LPSTR, 0x00525348> glpCmdLine;
    loco_global_array<char, 256, 0x005060D0> gCDKey;

    loco_global<uint16_t, 0x0050C19C> time_since_last_tick;
    loco_global<uint32_t, 0x0050C19E> last_tick_time;
    loco_global<uint8_t, 0x00508F08> game_command_nest_level;
    loco_global<uint16_t, 0x00508F12> _screen_age;
    loco_global<uint8_t, 0x00508F14> _screen_flags;
    loco_global<uint8_t, 0x00508F17> paused_state;
    loco_global<uint8_t, 0x00508F1A> game_speed;
    loco_global<uint8_t, 0x0050AF26> byte_50AF26;

    void tick_logic(int32_t count);
    void tick_logic();
    void tick_wait();

    void * hInstance()
    {
        return ghInstance;
    }

    const char * lpCmdLine()
    {
        return glpCmdLine;
    }

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

    bool sub_4054B9()
    {
        registers regs;
        LOCO_CALLFUNC_X(0x004054B9, regs);
        return regs.eax != 0;
    }

    /**
     * Use this to allocate memory that will be freed in vanilla code or via loco_free.
     */
    void * malloc(size_t size)
    {
        return ((void *(*)(size_t))0x004D1401)(size);
    }

    /**
     * Use this to reallocate memory that will be freed in vanilla code or via loco_free.
     */
    void * realloc(void * address, size_t size)
    {
        return ((void *(*)(void *, size_t))0x004D1B28)(address, size);
    }

    /**
     * Use this to free up memory allocated in vanilla code or via loco_malloc / loco_realloc.
     */
    void free(void * address)
    {
        ((void(*)(void *))0x004D1355)(address);
    }

    void sub_404E58()
    {
        free(LOCO_GLOBAL(0x005251F4, void *));
        LOCO_GLOBAL(0x005251F4, void *) = nullptr;
        LOCO_GLOBAL(0x005251F0, void *) = nullptr;
        LOCO_CALLPROC_X(0x00404ACD);
        LOCO_CALLPROC_X(0x00404B40);
    }

    void sub_4062D1()
    {
        LOCO_CALLPROC_X(0x004062D1);
    }

    void sub_406417()
    {
        ((void(*)())0x00406417)();
    }

    void sub_40567E()
    {
        LOCO_CALLPROC_X(0x0040567E);
    }

    void sub_4058F5()
    {
        LOCO_CALLPROC_X(0x004058F5);
    }

    void sub_4062E0()
    {
        LOCO_CALLPROC_X(0x004062E0);
    }

    // eax: width
    // ebx: height
    bool sub_451F0B(int32_t width, int32_t height)
    {
        registers regs;
        regs.eax = width;
        regs.ebx = height;
        LOCO_CALLFUNC_X(0x00451F0B, regs);
        return regs.al != 0;
    }

    void sub_4BE621(int32_t eax, int32_t ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        LOCO_CALLPROC_X(0x004BE621, regs);
    }

    void sub_45235D()
    {
        LOCO_CALLPROC_X(0x00452336);
        int32_t width = LOCO_GLOBAL(0x0050AEB8, int16_t);
        int32_t height = LOCO_GLOBAL(0x0050AEBA, int16_t);
        if (LOCO_GLOBAL(0x0050AEC0, uint8_t) != 0xFF || width == -1)
        {
            // int32_t screenWidth = LOCO_GLOBAL(0x00113E2C8, int32_t);
            int32_t screenHeight = LOCO_GLOBAL(0x00113E2CC, int32_t);
            width = 1024;
            height = 768;
            if (screenHeight < 1200)
            {
                width = 800;
                height = 600;
            }
        }
        if (sub_451F0B(width, height))
        {
            LOCO_GLOBAL(0x0052533C, int32_t) = 0;
            if (LOCO_GLOBAL(0x0052532C, int32_t) == 0 &&
                LOCO_GLOBAL(0x00113E2E4, int32_t) >= 64)
            {
                LOCO_CALLPROC_X(0x004524C1);
                LOCO_CALLPROC_X(0x004523F4);
            }
            else
            {
                sub_4BE621(61, 62);
            }
        }
        else
        {
            sub_4BE621(61, 63);
        }
    }

    // 0x00407FCD
    void get_cursor_pos(int32_t &x, int32_t &y)
    {
        POINT point;
        GetCursorPos(&point);
        x = point.x;
        y = point.y;
    }

    // 0x00407FEE
    void set_cursor_pos(int32_t x, int32_t y)
    {
        SetCursorPos(x, y);
    }

    bool sub_4034FC(int32_t &a, int32_t &b)
    {
        auto result = ((int32_t(*)(int32_t &, int32_t &))(0x004034FC))(a, b);
        return result != 0;
    }

    void sub_431A8A(uint16_t bx, uint16_t dx)
    {
        registers regs;
        regs.bx = bx;
        regs.dx = dx;
        LOCO_CALLPROC_X(0x00431A8A, regs);
    }

    // 0x004412CE
    void resolve_paths()
    {
        LOCO_CALLPROC_X(0x004412CE);
    }

    // 0x00407FFD
    bool is_already_running(const char * mutexName)
    {
        auto result = ((int32_t(*)(const char *))(0x00407FFD))(mutexName);
        return result != 0;
    }

    // 0x004BE621
    void exit_with_error(string_id eax, string_id ebx)
    {
        registers regs;
        regs.eax = eax;
        regs.ebx = ebx;
        LOCO_CALLPROC_X(0x004BE621, regs);
    }

    // 0x0044154B
    void check_game_files_exist()
    {
        LOCO_CALLPROC_X(0x0044154B);
    }

    // 0x004414C5
    void check_game_files_are_valid()
    {
        LOCO_CALLPROC_X(0x004414C5);
    }

    void sub_441444()
    {
        LOCO_CALLPROC_X(0x00441444);
    }

    // 0x00441400
    void startup_checks()
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
    void initialise_viewports()
    {
        LOCO_CALLPROC_X(0x004C57C0);
    }

    void initialise()
    {
        LOCO_GLOBAL(0x0050C18C, int32_t) = LOCO_GLOBAL(0x00525348, int32_t);
        LOCO_CALLPROC_X(0x004078BE);
        LOCO_CALLPROC_X(0x004BF476);
        resolve_paths();
        progressbar::begin(0x440, 0);
        progressbar::increment(0x1E);
        startup_checks();
        progressbar::increment(0x28);
        LOCO_CALLPROC_X(0x004BE5DE);
        progressbar::end();
        config::read();
        objectmgr::load_index();
        scenariomgr::load_index(0);
        progressbar::begin(0x440, 0);
        progressbar::increment(0x3C);
        gfx::load_g1();
        progressbar::increment(0xDC);
        LOCO_CALLPROC_X(0x004949BC);
        progressbar::increment(0xEB);
        progressbar::increment(0xFA);
        ui::initialise_cursors();
        progressbar::end();
        ui::initialise();
        audio::initialise();
        initialise_viewports();
        LOCO_CALLPROC_X(0x004284C8);
        LOCO_CALLPROC_X(0x004969DA);
        LOCO_CALLPROC_X(0x0043C88C);
        LOCO_GLOBAL(0x00508F14, int16_t) |= 0x20;
#ifdef _SHOW_INTRO_
        intro::state(intro::intro_state::begin);
#else
        intro::state(intro::intro_state::end);
#endif
        LOCO_CALLPROC_X(0x0046AD7D);
        LOCO_CALLPROC_X(0x00438A6C);
        gfx::clear(gfx::screen_dpi, 0x0A0A0A0A);
    }

    // 0x0046A794
    void tick()
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
        static loco_global<void *, 0x0050C1A6> tickJumpESP;
        static uint8_t spareStackMemory[2048];
        tickJumpESP = spareStackMemory + sizeof(spareStackMemory);

        if (setjmp(tickJump))
        {
            // Premature end of current tick
            std::cout << "tick prematurely ended" << std::endl;
            return;
        }

        LOCO_GLOBAL(0x00113E87C, int32_t) = 0;
        LOCO_GLOBAL(0x0005252E0, int32_t) = 0;
        if (!isInitialised)
        {
            isInitialised = true;

            // This address is where those routines jump back to to end the tick prematurely
            register_hook(0x0046AD71,
                [](registers &regs) -> uint8_t
                {
                    longjmp(tickJump, 1);
                });

            initialise();
            last_tick_time = timeGetTime();
        }

        uint32_t time = timeGetTime();
        time_since_last_tick = std::min(time - last_tick_time, 500U);
        last_tick_time = time;

        if (!is_paused())
        {
            LOCO_GLOBAL(0x0050C1A2, uint32_t) += time_since_last_tick;
        }
        if (tutorial::state() != tutorial::tutorial_state::none)
        {
            time_since_last_tick = 31;
        }
        game_command_nest_level = 0;
        ui::update();

        LOCO_GLOBAL(0x005233AE, int32_t) += LOCO_GLOBAL(0x0114084C, int32_t);
        LOCO_GLOBAL(0x005233B2, int32_t) += LOCO_GLOBAL(0x01140840, int32_t);
        LOCO_GLOBAL(0x0114084C, int32_t) = 0;
        LOCO_GLOBAL(0x01140840, int32_t) = 0;
        if (byte_50AF26 == 0)
        {
            byte_50AF26 = 16;
            gfx::clear(gfx::screen_dpi, 0);
            get_cursor_pos(LOCO_GLOBAL(0x00F2538C, int32_t), LOCO_GLOBAL(0x00F25390, int32_t));
            LOCO_GLOBAL(0x00F2539C, int32_t) = 0;
        }
        else
        {
            if (byte_50AF26 >= 16)
            {
                byte_50AF26++;
                if (byte_50AF26 >= 48)
                {
                    if (sub_4034FC(LOCO_GLOBAL(0x00F25394, int32_t), LOCO_GLOBAL(0x00F25398, int32_t)))
                    {
                        uintptr_t esi = LOCO_GLOBAL(0x00F25390, int32_t) + 4;
                        esi *= LOCO_GLOBAL(0x00F25398, int32_t);
                        esi += LOCO_GLOBAL(0x00F2538C, int32_t);
                        esi += 2;
                        esi += LOCO_GLOBAL(0x00F25394, int32_t);
                        LOCO_GLOBAL(0x00F2539C, int32_t) |= *((int32_t *)esi);
                        LOCO_CALLPROC_X(0x00403575);
                    }
                }
                set_cursor_pos(LOCO_GLOBAL(0x00F2538C, int32_t), LOCO_GLOBAL(0x00F25390, int32_t));
                gfx::invalidate_screen();
                if (byte_50AF26 != 96)
                {
                    tick_wait();
                    return;
                }
                byte_50AF26 = 1;
                if (LOCO_GLOBAL(0x00F2539C, int32_t) != 0)
                {
                    byte_50AF26 = 2;
                }
                config::write();
            }

            LOCO_CALLPROC_X(0x00452D1A);
            LOCO_CALLPROC_X(0x00440DEC);

            if (LOCO_GLOBAL(0x00525340, int32_t) == 1)
            {
                LOCO_GLOBAL(0x00525340, int32_t) = 0;
                LOCO_GLOBAL(0x00508F10, uint16_t) |= (1 << 1);
            }

            input::handle_keyboard();
            sub_48A18C();

            LOCO_GLOBAL(0x0050C1AE, int32_t)++;
            if (intro::is_active())
            {
                intro::update();
            }
            else
            {
                uint16_t numUpdates = std::clamp<uint16_t>(time_since_last_tick / (uint16_t)31, 1, 3);
                if (windowmgr::find(window_type::window_39, 0) != nullptr)
                {
                    numUpdates = 1;
                }
                if (_screen_flags & screen_flags::unknown_2)
                {
                    numUpdates = 1;
                }
                if (LOCO_GLOBAL(0x00525324, int32_t) == 1)
                {
                    LOCO_GLOBAL(0x00525324, int32_t) = 0;
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
                    }
                }
                LOCO_GLOBAL(0x0052622E, int16_t) += numUpdates;
                if (is_paused())
                {
                    numUpdates = 0;
                }
                LOCO_GLOBAL(0x00F253A0, uint16_t) = std::max<uint16_t>(1, numUpdates);
                _screen_age = std::min(0xFFFF, (int32_t)_screen_age + LOCO_GLOBAL(0x00F253A0, int16_t));
                if (game_speed != 0)
                {
                    numUpdates *= 3;
                    if (game_speed != 1)
                    {
                        numUpdates *= 3;
                    }
                }

                LOCO_CALLPROC_X(0x0046FFCA);
                tick_logic(numUpdates);

                LOCO_GLOBAL(0x00525F62, int16_t)++;
                LOCO_CALLPROC_X(0x0043D9D4);
                LOCO_CALLPROC_X(0x0048A78D);
                LOCO_CALLPROC_X(0x0048AC66);
                if (tutorial::state() != tutorial::tutorial_state::none &&
                    LOCO_GLOBAL(0x0052532C, int32_t) == 0 &&
                    LOCO_GLOBAL(0x0113E2E4, int32_t) < 0x40)
                {
                    tutorial::stop();

                    // This ends with a premature tick termination
                    LOCO_CALLPROC_X(0x0043C0FD);
                    return; // won't be reached
                }

                LOCO_CALLPROC_X(0x00431695);
                LOCO_CALLPROC_X(0x00452B5F);
                LOCO_CALLPROC_X(0x0046FFCA);
                if (LOCO_GLOBAL(0x0050AEC0, uint8_t) != 0xFF)
                {
                    LOCO_GLOBAL(0x0050AEC0, uint8_t)++;
                    if (LOCO_GLOBAL(0x0050AEC0, uint8_t) != 0xFF)
                    {
                        config::write();
                    }
                }
            }

            if (byte_50AF26 == 2)
            {
                LOCO_GLOBAL(0x005252DC, int32_t) = 1;
                get_cursor_pos(LOCO_GLOBAL(0x00F2538C, int32_t), LOCO_GLOBAL(0x00F25390, int32_t));
                set_cursor_pos(LOCO_GLOBAL(0x00F2538C, int32_t), LOCO_GLOBAL(0x00F25390, int32_t));
            }
        }

        tick_wait();
    }

    void tick_logic(int32_t count)
    {
        for (int32_t i = 0; i < count; i++)
        {
            tick_logic();
        }
    }

    // 0x004612EC
    void invalidate_map_animations()
    {
        LOCO_CALLPROC_X(0x004612EC);
    }

    // 0x0046ABCB
    void tick_logic()
    {
        LOCO_GLOBAL(0x00525F5E, int32_t)++;
        LOCO_GLOBAL(0x00525F64, int32_t)++;
        LOCO_GLOBAL(0x00525FCC, int32_t) = LOCO_GLOBAL(0x00525E18, int32_t);
        LOCO_GLOBAL(0x00525FD0, int32_t) = LOCO_GLOBAL(0x00525E1C, int32_t);
        LOCO_CALLPROC_X(0x004613F0);
        LOCO_GLOBAL(0x00F25374, uint8_t) = LOCO_GLOBAL(0x009C871C, uint8_t);
        LOCO_CALLPROC_X(0x004968C7);
        LOCO_CALLPROC_X(0x00463ABA);
        LOCO_CALLPROC_X(0x004C56F6);
        LOCO_CALLPROC_X(0x00496B6D);
        LOCO_CALLPROC_X(0x00453234);
        thingmgr::update_vehicles();
        LOCO_CALLPROC_X(0x0046FFCA);
        LOCO_CALLPROC_X(0x0048B1FA);
        thingmgr::update_misc_things();
        LOCO_CALLPROC_X(0x0046FFCA);
        LOCO_CALLPROC_X(0x00430319);
        invalidate_map_animations();
        LOCO_CALLPROC_X(0x0048A73B);
        LOCO_CALLPROC_X(0x0048ACFD);
        LOCO_CALLPROC_X(0x00444387);

        LOCO_GLOBAL(0x009C871C, uint8_t) = LOCO_GLOBAL(0x00F25374, uint8_t);
        if (LOCO_GLOBAL(0x0050C197, uint8_t) != 0)
        {
            uint16_t bx = 0x043A;
            uint16_t dx = LOCO_GLOBAL(0x0050C198, uint16_t);
            if (LOCO_GLOBAL(0x0050C197, uint8_t) == 0xFE)
            {
                bx = LOCO_GLOBAL(0x0050C198, uint16_t);
                dx = 0xFFFF;
            }
            LOCO_GLOBAL(0x0050C197, uint8_t) = 0;
            sub_431A8A(bx, dx);
        }
    }

    // 0x0046AD4D
    void tick_wait()
    {
        do
        {
            // Idle loop for a 40 FPS
        }
        while (timeGetTime() - last_tick_time < 25);
    }

    void prompt_tick_loop(std::function<bool()> tickAction)
    {
        while (true)
        {
            auto startTime = timeGetTime();
            time_since_last_tick = 31;
            if (!ui::process_messages() || !tickAction())
            {
                break;
            }
            ui::render();
            do
            {
                // Idle loop for a 40 FPS
            }
            while (timeGetTime() - startTime < 25);
        }
    }

    void sub_48A18C()
    {
        LOCO_CALLPROC_X(0x0048A18C);
    }

    // 0x00406386
    void run()
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        sub_4062D1();
        sub_406417();

#if _READ_REGISTRY_
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
            if (LOCO_GLOBAL(0x005252AC, uint32_t) != 0)
            {
                sub_4058F5();
            }
            sub_4062E0();
            tick();
            ui::render();
        }
        sub_40567E();
        CoUninitialize();
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
                LOCO_CALLPROC_X(0x004078FE);
                LOCO_CALLPROC_X(0x00407B26);
                ui::initialise_input();
                audio::initialise_dsound();
                run();
                audio::dispose_dsound();
                ui::dispose_input();

                // TODO extra clean up code
            }
        }
        catch (const std::exception &ex)
        {
            std::cerr << ex.what() << std::endl;
        }
    }
}

extern "C"
{
    /**
     * The function that is called directly from the host application (loco.exe)'s WinMain. This will be removed when OpenLoco can
     * be built as a stand alone application.
     */
    __declspec(dllexport) int StartOpenLoco(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        openloco::glpCmdLine = lpCmdLine;
        openloco::ghInstance = hInstance;
        openloco::main();
        return 0;
    }
}
