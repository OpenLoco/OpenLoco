#include <iostream>
#include <string>
#include <vector>
#include <setjmp.h>

// timeGetTime is unavailable if we use lean and mean
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>

#include "graphics/gfx.h"
#include "interop/interop.hpp"
#include "progressbar.h"

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non - portable

namespace openloco
{
    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";

    loco_global<HINSTANCE, 0x0113E0B4> ghInstance;
    loco_global<LPSTR, 0x00525348> glpCmdLine;
    loco_global<HWND, 0x00525320> gMainHWND;
    loco_global_array<char, 256, 0x005060D0> gCDKey;
    loco_global<void *, 0x0050C1A6> gTickESP;

    loco_global<uint8_t, 0x0050C195> gIntroState;

    // 0x00405409
    HWND create_game_window()
    {
        return CreateWindowExA(
            WS_EX_TOPMOST,
            WINDOW_CLASS_NAME,
            WINDOW_TITLE,
            WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_CLIPCHILDREN | WS_MAXIMIZE | WS_CLIPSIBLINGS,
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN),
            nullptr,
            nullptr,
            ghInstance,
            nullptr);
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

    bool process_messages()
    {
        return ((bool(*)())0x0040726D)();
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

    void sub_44452F(int8_t al)
    {
        registers regs;
        regs.al = al;
        LOCO_CALLPROC_X(0x0044452F, regs);
    }

    // 0x0044733C
    void load_g1()
    {
        LOCO_CALLPROC_X(0x0044733C);
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
        static uint8_t spareStackMemory[2048];
        gTickESP = spareStackMemory + sizeof(spareStackMemory);

        LOCO_GLOBAL(0x00113E87C, int32_t) = 0;
        LOCO_GLOBAL(0x0005252E0, int32_t) = 0;
        if (!isInitialised)
        {
            isInitialised = true;
            LOCO_GLOBAL(0x0050C18C, int32_t) = LOCO_GLOBAL(0x00525348, int32_t);
            LOCO_CALLPROC_X(0x004078BE);
            LOCO_CALLPROC_X(0x004BF476);
            LOCO_CALLPROC_X(0x004412CE);
            progressbar::begin(0x440, 0);
            progressbar::increment(0x1E);
            LOCO_CALLPROC_X(0x00441400); // double instance check is in here
            progressbar::increment(0x28);
            LOCO_CALLPROC_X(0x004BE5DE);
            progressbar::end();
            LOCO_CALLPROC_X(0x00441A6C);
            LOCO_CALLPROC_X(0x00470F3C);
            sub_44452F(0);
            progressbar::begin(0x440, 0);
            progressbar::increment(0x3C);
            load_g1();
            progressbar::increment(0xDC);
            LOCO_CALLPROC_X(0x004949BC);
            progressbar::increment(0xEB);
            progressbar::increment(0xFA);
            LOCO_CALLPROC_X(0x00452001);
            progressbar::end();
            sub_45235D();
            LOCO_CALLPROC_X(0x004899E4);
            LOCO_CALLPROC_X(0x004C57C0);
            LOCO_CALLPROC_X(0x004284C8);
            LOCO_CALLPROC_X(0x004969DA);
            LOCO_CALLPROC_X(0x0043C88C);
            LOCO_GLOBAL(0x00508F14, int16_t) |= 0x20;
            gIntroState = 1;
            LOCO_CALLPROC_X(0x0046AD7D);
            LOCO_CALLPROC_X(0x00438A6C);
            gfx::clear(gfx::screen_dpi, 0x0A0A0A0A);
            LOCO_GLOBAL(0x0050C19E, int32_t) = timeGetTime();
        }

        // CONTINUE FUNCTION
        static bool registeredHooks = false;
        if (!registeredHooks)
        {
            registeredHooks = true;

            // Jump from vanilla back to us before the end of the routine
            register_hook(0x0046AD4D,
                [](const registers &regs) -> uint8_t
                {
                    // Idle loop for a 40 FPS
                    do
                    {
                    }
                    while (timeGetTime() - LOCO_GLOBAL(0x0050C19E, uint32_t) < 25);
                    return 0;
                });

            // This address is where those routines jump back to to end the tick prematurely
            register_hook(0x0046AD71,
                [](const registers &regs) -> uint8_t
                {
                    longjmp(tickJump, 1);
                });

        }

        if (!setjmp(tickJump))
        {
            // Execute vanilla chunk of tick()
            LOCO_CALLPROC_X(0x0046A8DD);
        }
        else
        {
            // Premature end of current tick
            std::cout << "tick prematurely ended" << std::endl;
        }
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

        while (process_messages())
        {
            if (LOCO_GLOBAL(0x005252AC, uint32_t) != 0)
            {
                sub_4058F5();
            }
            sub_4062E0();
            tick();
        }
        sub_40567E();
        CoUninitialize();
    }

    // 0x00406D13
    void main()
    {
        std::cout << "OpenLoco v0.1" << std::endl;
        if (sub_4054B9())
        {
            gMainHWND = create_game_window();
            LOCO_CALLPROC_X(0x004078FE);
            LOCO_CALLPROC_X(0x00407B26);
            LOCO_CALLPROC_X(0x0040447F);
            LOCO_CALLPROC_X(0x00404E53);
            run();
            LOCO_CALLPROC_X(0x00404E58);
            LOCO_CALLPROC_X(0x004045C2);

            // TODO extra clean up code
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
