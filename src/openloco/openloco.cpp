#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "interop\interop.hpp"

namespace openloco
{
    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";

    loco_global<HINSTANCE, 0x0113E0B4> ghInstance;
    loco_global<LPSTR, 0x00525348> glpCmdLine;
    loco_global<HWND, 0x00525320> gMainHWND;

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

    // 0x00406386
    void openloco_run()
    {
        LOCO_CALLPROC_X(0x00406386);
    }

    // 0x00406D13
    void main()
    {
        if (sub_4054B9())
        {
            gMainHWND = create_game_window();
            LOCO_CALLPROC_X(0x004078FE);
            LOCO_CALLPROC_X(0x00407B26);
            LOCO_CALLPROC_X(0x0040447F);
            LOCO_CALLPROC_X(0x00404E53);
            openloco_run();
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
