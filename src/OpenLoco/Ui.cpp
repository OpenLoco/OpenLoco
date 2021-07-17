#include "Ui/Cursor.h"
#include "Win32.h"
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include "../../resources/Resource.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

// `small` is used as a type in `windows.h`
#undef small
#endif

#ifndef _LOCO_WIN32_
#include <SDL2/SDL.h>
#pragma warning(disable : 4121) // alignment of a member was sensitive to packing
#include <SDL2/SDL_syswm.h>
#pragma warning(default : 4121) // alignment of a member was sensitive to packing
#endif

#include "Config.h"
#include "Console.h"
#include "Drawing/FPSCounter.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Gfx.h"
#include "Gui.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "Intro.h"
#include "MultiPlayer.h"
#include "OpenLoco.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "Utility/String.hpp"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Ui
{
#ifdef _LOCO_WIN32_
    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";
#endif // _WIN32

#pragma pack(push, 1)

    struct palette_entry_t
    {
        uint8_t b, g, r, a;
    };

    struct sdl_window_desc
    {
        int32_t x{};
        int32_t y{};
        int32_t width{};
        int32_t height{};
        int32_t flags{};
    };

#pragma pack(pop)

    using set_palette_func = void (*)(const palette_entry_t* palette, int32_t index, int32_t count);

#ifdef _WIN32
    loco_global<void*, 0x00525320> _hwnd;
#endif // _WIN32
    loco_global<ScreenInfo, 0x0050B884> screen_info;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIdx;
    loco_global<set_palette_func, 0x0052524C> set_palette_callback;
    static loco_global<uint32_t, 0x00525E28> _525E28;
    loco_global<uint8_t[256], 0x01140740> _keyboard_state;

    bool _resolutionsAllowAnyAspectRatio = false;
    std::vector<Resolution> _fsResolutions;

    static SDL_Window* window;
    static SDL_Surface* surface;
    static SDL_Surface* RGBASurface;
    static SDL_Palette* palette;
    static std::vector<SDL_Cursor*> _cursors;

    static void setWindowIcon();
    static void update(int32_t width, int32_t height);
    static void resize(int32_t width, int32_t height);
    static int32_t convertSdlKeycodeToWindows(int32_t keyCode);
    static Config::Resolution getDisplayResolutionByMode(Config::ScreenMode mode);

#if !(defined(__APPLE__) && defined(__MACH__))
    static void toggleFullscreenDesktop();
#endif

#ifdef _WIN32
    void* hwnd()
    {
        return _hwnd;
    }
#endif // _WIN32

    int32_t width()
    {
        return screen_info->width;
    }

    int32_t height()
    {
        return screen_info->height;
    }

    bool dirtyBlocksInitialised()
    {
        return screen_info->dirty_blocks_initialised != 0;
    }

    void updatePalette(const palette_entry_t* entries, int32_t index, int32_t count);

    static sdl_window_desc getWindowDesc(const Config::Display& cfg)
    {
        sdl_window_desc desc;
        desc.x = SDL_WINDOWPOS_CENTERED_DISPLAY(cfg.index);
        desc.y = SDL_WINDOWPOS_CENTERED_DISPLAY(cfg.index);
        desc.width = std::max(640, cfg.window_resolution.width);
        desc.height = std::max(480, cfg.window_resolution.height);
        desc.flags = SDL_WINDOW_RESIZABLE;
#if !(defined(__APPLE__) && defined(__MACH__))
        switch (cfg.mode)
        {
            case Config::ScreenMode::window:
                break;
            case Config::ScreenMode::fullscreen:
                desc.width = cfg.fullscreen_resolution.width;
                desc.height = cfg.fullscreen_resolution.height;
                desc.flags |= SDL_WINDOW_FULLSCREEN;
                break;
            case Config::ScreenMode::fullscreenBorderless:
                desc.flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                break;
        }
#endif
        return desc;
    }

    // 0x00405409
    void createWindow(const Config::Display& cfg)
    {
#ifdef _LOCO_WIN32_
        _hwnd = CreateWindowExA(
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
            (HINSTANCE)hInstance(),
            nullptr);
#else
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw std::runtime_error("Unable to initialise SDL2 video subsystem.");
        }

        // Create the window
        auto desc = getWindowDesc(cfg);
        window = SDL_CreateWindow("OpenLoco", desc.x, desc.y, desc.width, desc.height, desc.flags);
        if (window == nullptr)
        {
            throw std::runtime_error("Unable to create SDL2 window.");
        }

#ifdef _WIN32
        // Grab the HWND
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
        {
            throw std::runtime_error("Unable to fetch SDL2 window system handle.");
        }
        _hwnd = wmInfo.info.win.window;
#endif

        setWindowIcon();

        // Create a palette for the window
        palette = SDL_AllocPalette(256);
        set_palette_callback = updatePalette;

        update(desc.width, desc.height);
#endif
    }

    static void setWindowIcon()
    {
#ifdef _WIN32
        auto win32module = GetModuleHandleA("openloco.dll");
        if (win32module != nullptr)
        {
            auto icon = LoadIconA(win32module, MAKEINTRESOURCEA(IDI_ICON));
            if (icon != nullptr)
            {
                auto hwnd = (HWND)*_hwnd;
                if (hwnd != nullptr)
                {
                    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
                }
            }
        }
#endif
    }

    // 0x0045235D
    void initialise()
    {
#ifdef _LOCO_WIN32_
        call(0x0045235D);
#endif
        SDL_RestoreWindow(window);
    }

    static Cursor cursor116 = Cursor(
        0, 0,
        "X                               "
        "XX                              "
        "X.X                             "
        "X..X                            "
        "X...X                           "
        "X....X                          "
        "X.....X                         "
        "X......X                        "
        "X.......X                       "
        "X........X                      "
        "X.....XXXXX                     "
        "X..X..X                         "
        "X.X X..X                        "
        "XX  X..X                        "
        "X    X..X                       "
        "     X..X                       "
        "      X..X                      "
        "      X..X                      "
        "       XX                       "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor165 = Cursor(
        0, 0,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor109 = Cursor(
        16, 0,
        "                X               "
        "               X.X              "
        "              X...X             "
        "             X.....X            "
        "            X.......X           "
        "           X.........X          "
        "           XXXX...XXXX          "
        "              X...X             "
        "              X...X             "
        "              X...X             "
        "              X...X             "
        "              X...X             "
        "              XXXXX             "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor110 = Cursor(
        16, 16,
        "                                "
        "                X               "
        "               X.X              "
        "              X.X.X             "
        "             X.X X.X            "
        "            X.X   X.X           "
        "           X.XXX XXX.X          "
        "          X....X X....X         "
        "           XXX.XXX.XXX          "
        "             X.....X            "
        "              XXXXX             "
        "                                "
        "                                "
        "                X               "
        "               X.X              "
        "              X...X             "
        "               X.X              "
        "                X               "
        "                                "
        "                                "
        "              XXXXX             "
        "             X.....X            "
        "             X.XXX.X            "
        "           XXX.X X.XXX          "
        "          X....X X....X         "
        "           X.XXX XXX.X          "
        "            X.X   X.X           "
        "             X.X X.X            "
        "              X.X.X             "
        "               X.X              "
        "                X               "
        "                                ");

    static Cursor cursor112 = Cursor(
        9, 3,
        "                                "
        "                                "
        "                                "
        "         XX                     "
        "        X..X                    "
        "        X..X                    "
        "        X..X                    "
        "        X..X                    "
        "        X..XXX                  "
        "        X..X..XXX               "
        "        X..X..X..XX             "
        "        X..X..X..X.X            "
        "    XXX X..X..X..X..X           "
        "    X..XX........X..X           "
        "    X...X...........X           "
        "     X..X...........X           "
        "      X.X...........X           "
        "      X.............X           "
        "       X............X           "
        "       X...........X            "
        "        X..........X            "
        "        X..........X            "
        "         X........X             "
        "         X........X             "
        "         XXXXXXXXXX             "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor120 = Cursor(
        16, 17,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                           XXX  "
        "                      XXXXX...X "
        "                XXXXXX....XX.X  "
        "               X.....XXX.XX...X "
        "          XXXXXXXXX.X X.XX XXX  "
        "         X......XX.X X....X     "
        "          XXXX.XX.XXX XXXX      "
        "  XXXXXXXX  X.XX.....X          "
        " X........XX.X  XXXXX           "
        " X........X.XXXX                "
        "  XXXX...X......X               "
        "    X...X XXXXXX                "
        "   X...X                        "
        "  X...XXXX                      "
        " X........X                     "
        " X........X                     "
        "  XXXXXXXX                      "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor119 = Cursor(
        7, 7,
        ".......                         "
        ".XXXX.                          "
        ".XXX.                           "
        ".XXX.                           "
        ".X..X.                          "
        "..  .X.                         "
        ".    .X.                        "
        "      .X.                       "
        "       .X.    .                 "
        "        .X.  ..                 "
        "         .X..X.                 "
        "          .XXX.                 "
        "          .XXX.                 "
        "         .XXXX.                 "
        "        .......                 "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor124 = Cursor(
        15, 31,
        "             XXXXX              "
        "            X....XX             "
        "           X..XX..XX            "
        "           X.XXXX.XX            "
        "           X.XXXX.XX            "
        "           X..XX..XX            "
        "            X....XX             "
        "            XXXXXXX             "
        "            XXXXXXX             "
        "           X.XXX..XX            "
        "           X..X...XX            "
        "          X....X...XX           "
        "          X...XX...XX           "
        "         X...X  X...XX          "
        "         X...X  X...XX          "
        "        X...X    X...XX         "
        "        X..X      X..XX         "
        "       X...X      X...XX        "
        "       X..X        X..XX        "
        "      X..X          X..XX       "
        "      X..X          X..XX       "
        "     X..X            X..XX      "
        "     X..X            X..XX      "
        "     X..X            X..XX      "
        "     X..X            X..XX      "
        "     X..X            X..XX      "
        "     X...X          X...XX      "
        "      X..X          X..XX       "
        "       X..X        X..XX        "
        "        X..X      X..XX         "
        "         XX.X    X.XXX          "
        "           XXX  XXXX            ");

    static Cursor cursor131 = Cursor(
        6, 31,
        "                                "
        "                 XXX            "
        "               XX...X  XXX      "
        "              X......XX...X     "
        "             XXX...........XX   "
        "             XXX.X...........X  "
        "              XXX............X  "
        "               XXX............X "
        "              XXX.....X.......X "
        "              XXXX.X.........X  "
        "             XXXXXXXX..X....X   "
        "            XXXX.XX.XXX......X  "
        "            XXX.X.XXX...X.....X "
        "            XXXX.X...X....X...X "
        "            XXXXX.X.X....X...XX "
        "            XXXXXXXXXX.....X.XX "
        "             XXXXXX.XXX.X...XX  "
        "    XXXXX      XXXXXXXXXXX.XX   "
        "    X...X        XXXXX.XXXXX    "
        "    X...X         XXX.X         "
        "    X...X          XX.X         "
        "    X...X          XX.X         "
        "    X...X          X..X         "
        "    X...X          X..X         "
        "    X...X          X...X        "
        "XXXXX...XXXXX    XX.....X       "
        " X.........X   XX........XX     "
        "  X.......X   X.....X..XX..X    "
        "   X.....X     XXX.X X.X XX     "
        "    X...X         X   X         "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor127 = Cursor(
        6, 31,
        "               .     .          "
        "           .       .    .      ."
        "                 .     .   . .  "
        "             . .    .     .     "
        "                  .   . .      ."
        "                .   .      .    "
        "                   .     .      "
        "                      .         "
        "                    .   .       "
        "                      .         "
        "                     X          "
        "                    XXX         "
        "                    X.X         "
        "                 XXXX.XXXX      "
        "               XX...X.XX..XX    "
        "              X....X.XXXXX..X   "
        "              X.....XXXXX...X   "
        "    XXXXX     XXX.........XXX   "
        "    X...X     X..XXXXXXXXX.XX   "
        "    X...X      X.......XXXXX    "
        "    X...X       XXX...XXXXX     "
        "    X...X         XXXXXXX       "
        "    X...X          X.XXX        "
        "    X...X          X..XX        "
        "    X...X          X..XX        "
        "XXXXX...XXXXX     X..XXXX       "
        " X.........X     X..XXX.XX      "
        "  X.......X     X.......XXX     "
        "   X.....X      XX.....XX.X     "
        "    X...X        XXXXXXXXX      "
        "     X.X           XXXXX        "
        "      X                         ");

    static Cursor cursor128 = Cursor(
        6, 31,
        "                                "
        "                                "
        "             X             X    "
        "             XXX      XX  XX    "
        "              X.X    X..X  XX   "
        "               X.X   X..X  XX   "
        "                X.XX X.XX X.X   "
        "                 X..X.X XX.X    "
        "                  X....X..X     "
        "                  X......X      "
        "                   X....X       "
        "                   X....X       "
        "                    X..X        "
        "                    X...X       "
        "                    X...X       "
        "                     X.X        "
        "                     X.X        "
        "    XXXXX           X...X       "
        "    X...X            X..X       "
        "    X...X            X.X        "
        "    X...X           XX.X        "
        "    X...X           X..X        "
        "    X...X         XXXXXXX       "
        "    X...X         X.....X       "
        "    X...X         X.....X       "
        "XXXXX...XXXXX     X.....X       "
        " X.........X      X.....X       "
        "  X.......X       X.....X       "
        "   X.....X       XXXXXXXXX      "
        "    X...X       X.........X     "
        "     X.X        XXXXXXXXXXX     "
        "      X                         ");

    static Cursor cursor129 = Cursor(
        6, 31,
        "                                "
        "                                "
        "             XX                 "
        "             X.XX               "
        "             X...XX             "
        "             X.....XX           "
        "             XX......XX         "
        "             X.XX......XX       "
        "             X.X XX......XX     "
        "            XX.X   XX......XX   "
        "          XX..XX     XX......X  "
        "        XX......XX     XX....X  "
        "        XXXX......XX     XX..X  "
        "        X.X XX......XX     XXX  "
        "        X.X  XXX......XX   X.X  "
        "         X      XX......XX X.X  "
        "                XXXX......XX.X  "
        "    XXXXX       X.X XX......XX  "
        "    X...X       X.X   XX..XXXX  "
        "    X...X        X     XXX X.X  "
        "    X...X              XXX X.X  "
        "    X...X              X.X  X   "
        "    X...X              X.X      "
        "    X...X               X       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor130 = Cursor(
        14, 16,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "              X                 "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "              X                 "
        "                                "
        "    XXXXXXX   X   XXXXXXX       "
        "   X.......X X.X X.......X      "
        "    XXXXXXX   X   XXXXXXX       "
        "                                "
        "              X                 "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "             X.X                "
        "              X                 "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor132 = Cursor(
        6, 31,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                   XXX          "
        "                  X   X         "
        "                  XXXXX         "
        "                XX....XXX       "
        "               X........XX      "
        "              X..X.X.XXXXXX     "
        "              X..........XX     "
        "              XXXXXXXXXXXXX     "
        "               X.......XXX      "
        "               X.X......XX      "
        "               X.X.X....XX      "
        "    XXXXX      X.X.X.....X      "
        "    X...X      X.X.X...X.X      "
        "    X...X      X.X.X...X.X      "
        "    X...X      X.X.X.X.X.X      "
        "    X...X      X.X.X.X.X.X      "
        "    X...X      X.X.X.X.X.X      "
        "    X...X      X.X.X.X.X.X      "
        "    X...X      X.X.X.X.X.X      "
        "XXXXX...XXXXX  X.X.X.X.X.X      "
        " X.........X   X.X.X.X.X.X      "
        "  X.......X    XXXXXXXXXXX      "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor133 = Cursor(
        6, 31,
        "                   XXX          "
        "                  X...X         "
        "                 X.....X        "
        "                XXXXXXXXX       "
        "                 XXXX..X        "
        "                 XX....X        "
        "                 XX....X        "
        "                  XX..X         "
        "                  XXXXX         "
        "                   XXX          "
        "                   X.X          "
        "                X  X.X  X       "
        "               XXXXX.XXXXX      "
        "                X  X.X  X       "
        "                   X.X          "
        "                   X.X          "
        "                   X.X          "
        "    XXXXX          X.X          "
        "    X...X          X.X          "
        "    X...X          X.X          "
        "    X...X          X.X          "
        "    X...X          X.X          "
        "    X...X          X.X          "
        "    X...X          XXX          "
        "    X...X          XXX          "
        "XXXXX...XXXXX     XX..X         "
        " X.........X     XX....X        "
        "  X.......X      XX....X        "
        "   X.....X       XXX...X        "
        "    X...X        XXXXX.X        "
        "     X.X        XXXXXXXXX       "
        "      X                         ");

    static Cursor cursor138 = Cursor(
        6, 31,
        "                                "
        "   XX                           "
        "  X.XX                          "
        " X..XXX  XX                     "
        " X....XXX.XX                    "
        "  X.....X.XX   XX               "
        "  X.XX....XXX X.XX              "
        "  X.XXXX....XXX.XX   XX         "
        "  X.XX  X.....X.XX  X.XX        "
        "  X.XX  X.XX.....XX X.XX   XX   "
        " XX.XX  X.XXXX.....XX.XX  X.XX  "
        " XXXXX  X.XX  X.X.....XX  X.XX  "
        "   XXXX X.XX  X.XXX....XX X.XX  "
        "     XXXX.XX  X.XX XX....XX.XX  "
        "       XXXXX  X.XX  X.X.....XX  "
        "         XXXX X.XX  X.XXX....XX "
        "           XXXX.XX  X.XX XX..XX "
        "    XXXXX    XXXXX  X.XX  X.XXX "
        "    X...X      XXXX X.XX  X.XX  "
        "    X...X        XXXX.XX  X.XX  "
        "    X...X          XXXXX  X.XX  "
        "    X...X            XXXX X.XX  "
        "    X...X              XXXX.XX  "
        "    X...X                XXXX   "
        "    X...X                  XX   "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor137 = Cursor(
        6, 31,
        "                   X            "
        "                  X.X           "
        "                 X...X          "
        "           XX    X...X    XX    "
        "          X..X   X.X.X   X..X   "
        "          X...X  X.X.X  X...X   "
        "           X...X X.X.X X...X    "
        "            X.X.X XXX X.X.X     "
        "             X.XX XXX XX.X      "
        "              XXXX...XXXX       "
        "         XXXXX  X..X..X  XXXXX  "
        "        X.....XX.X...XXXX.....X "
        "       X...XXX.XX...X..XXXXX...X"
        "        X.....XX..X...XXX.....X "
        "         XXXXX  X...X.X  XXXXX  "
        "              XXXXX..XXXX       "
        "             X.XX XXX XX.X      "
        "    XXXXX   X.X.X XXX X.X.X     "
        "    X...X  X...X X.X.X X...X    "
        "    X...X  X..X  X.X.X  X..X    "
        "    X...X   XX   X...X   XX XX  "
        "    X...X  XX    X...X    XX..X "
        "    X...X X..XX  X...X   X....X "
        "    X...X X....X  X.X   X.....X "
        "    X...X X.....X XXX  X..X..X  "
        "XXXXX...XXXXX.X.X XXX X..X...X  "
        " X.........X...X.XX.X X.X...X   "
        "  X.......X X...XXX.XX.X...X    "
        "   X.....X   XX..X..X.X..XX     "
        "    X...X      XX......XX       "
        "     X.X         XXXXXX         "
        "      X                         ");

    static Cursor cursor139 = Cursor(
        6, 31,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "           XX                   "
        "         XX..XX                 "
        "       XX....X.XX               "
        "     XX..X..X....XX             "
        "   XX.X...X..X...X.XX           "
        " XX...X...XXX.X..X...XX         "
        " XXXX..XXX.....X.X...X.XX       "
        "   XXXX...X..XXXXXXXXX...XX     "
        "     XXXX..XX...X.....X..X.XX   "
        "       XXXX.....X.....X..X...XX "
        "         XXXX..X.X...X.X.XXXX..X"
        "           XXXX...X.X...X....XXX"
        "    XXXXX    XXXX..X.X..X..XXXX "
        "    X...X      XXXX...X..XXXX   "
        "    X...X        XXXX..XXXX     "
        "    X...X          XXXXXX       "
        "    X...X            XX         "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor141 = Cursor(
        6, 31,
        "                                "
        "                          XX    "
        "                         X..X   "
        "                        X.X..X  "
        "                       X.X X..X "
        "                      X.X   X.XX"
        "                      X.X  X.XX "
        "                      X..XX.XX  "
        "                     X.....XX   "
        "                    X...XXXX    "
        "                   X...XX       "
        "              XX  X.X.X         "
        "             X..XX...XX         "
        "            X..XX...XX          "
        "           X.X.X...XX           "
        "          X...X...XX            "
        "          X..X...XXXX           "
        "    XXXXXX...X.XXXX..X          "
        "    X...XX...XXXXX...X          "
        "    X...XX....XX..X.X           "
        "    X...X X........X            "
        "    X...X  X......X             "
        "    X...X   X...XX              "
        "    X...X    XXX                "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor142 = Cursor(
        6, 31,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "    X     X     X     X         "
        "    X     X     X     X         "
        "   X.X   X.X   X.X   X.X        "
        " XX...XXX...XXX...XXX...XX      "
        "X...X.....X.....X.....X...X     "
        " XXX XXXXX XXXXX XXXXX XXX      "
        "                                "
        "          X     X     X     X   "
        "          X     X     X     X   "
        "         X.X   X.X   X.X   X.X  "
        "        X...XXX...XXX...XXX...X "
        "    XXXXX.X.....X.....X.....X..X"
        "    X...XX XXXXX XXXXX XXXXX XX "
        "    X...X                       "
        "    X...X       X     X         "
        "    X...X       X     X         "
        "    X...X      X.X   X.X        "
        "    X...X    XX...XXX...XX      "
        "XXXXX...XXXXX...X.....X...X     "
        " X.........X XXX XXXXX XXX      "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor143 = Cursor(
        6, 31,
        "                                "
        "                                "
        "                   X  XXXXXX    "
        "                  X.X X....X    "
        "                 X...X XXXX     "
        "                X..X..XX..X     "
        "               X...XX..X..X     "
        "              X.....XX....X     "
        "             X...XXXXXX...X     "
        "            X.........XX..X     "
        "           X....XXXXXXXXX..X    "
        "          X.............XX..X   "
        "         X...XXXXXXXXXXXXXX..X  "
        "        XXXX.............XXXXXX "
        "           X...............X    "
        "           X...............X    "
        "           X...XXXX.XXXXXX.X    "
        "           X...X  X.X    X.X    "
        "    XXXXX  X...X  X.X    X.X    "
        "    X...X  X...X  X.X    X.X    "
        "    X...X  X...X  X.XXXXXX.X    "
        "    X...X  X...X  X........X    "
        "    X...X  X...X  X........X    "
        "    X...X  XXXXXXXXXXXXXXXXX    "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor144 = Cursor(
        6, 31,
        "            X   X X             "
        "              X     X           "
        "                 X              "
        "              X  X              "
        "             XXXX XX            "
        "           XXXXXXXXXXX          "
        "           X.XXXX....X          "
        "          XX.......XXXX         "
        "         XXX.XX..XXX..X         "
        "         X.XX.X.X...X..X        "
        "        X......X...XXX..X       "
        "      XX....X.......X.X..XXX    "
        "     X.....XX......X.XXXX...X   "
        "   XX.....XX........X.X.X.X..XX "
        "  X...XXXX....X......XXX.XXXXXXX"
        " XXXXX   X.XXXXXX...XXXXXXX XX  "
        " X      XXXX    XX.XX   XX      "
        "         X        X             "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor145 = Cursor(
        6, 31,
        "                                "
        "      XXX                       "
        "     X...X                      "
        "     X....X                     "
        "      X....X                    "
        "       X...X                    "
        "        XXX  X                  "
        "           XX.X                 "
        "           X..X                 "
        "            XX                  "
        "                                "
        "       XXX         XXX          "
        "      X...X       X...X         "
        "      X....X      X....X        "
        "       X....X      X....X       "
        "        X...X       X...X       "
        "         XXX  X      XXX  X     "
        "            XX.X        XX.X    "
        "    XXXXX   X..X        X..X    "
        "    X...X    XX          XX     "
        "    X...X                       "
        "    X...X           XXX         "
        "    X...X          X...X        "
        "    X...X          X....X       "
        "    X...X           X....X      "
        "XXXXX...XXXXX        X...X      "
        " X.........X          XXX  X    "
        "  X.......X              XX.X   "
        "   X.....X               X..X   "
        "    X...X                 XX    "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor158 = Cursor(
        7, 30,
        "                                "
        "                           XXX  "
        "                         XXXXXX "
        "                        XXXXX.X "
        "                       XXXXX.XX "
        "                      XXXXX..X  "
        "              XX     XXXXX..XX  "
        "             XXXX    X..X..XX   "
        "            XXXXXX  X....XXX    "
        "           XX.XXXXXXX....XX     "
        "          XXXX.XXXX.....XX      "
        "         XXXXXX.XXX...XX        "
        "        XXXXXXXX.XX...X         "
        "        XX.XXXXXX.XXXXX         "
        "         XXXXXXXXX.XXXXX        "
        "          XX.XXXXXX.XXXXX       "
        "           XX.XXXXXX.XXXXX      "
        "     XXXXX  XXXXXXXXX.XXXX      "
        "     X...X   XX.X..XXX.XXX      "
        "     X...X    XX.....XXXX       "
        "     X...X     XX.....XX        "
        "     X...X      XX...XX         "
        "     X...X       XX.XX          "
        "     X...X        X.X           "
        " XXXXX...XXXXX     X            "
        "  X.........X                   "
        "   X.......X                    "
        "    X.....X                     "
        "     X...X                      "
        "      X.X                       "
        "       X                        "
        "                                ");

    static Cursor cursor159 = Cursor(
        6, 31,
        "  X                         X   "
        " X.X                       X.X  "
        "X...X                     X...X "
        " X.X                       X.X  "
        " XXXXXXXXXXXXXXXXXXXXXXXXXXXXX  "
        " X.XX.....................XX.X  "
        " X.X X...................X X.X  "
        " X.X X...................X X.X  "
        " X.X X...................X X.X  "
        " X.XXX....................XX.X  "
        " X.XXXXXXXXXXXXXXXXXXXXXXXXX.X  "
        " X.X                       X.X  "
        " X.X                       X.X  "
        " X.X                       X.X  "
        " X.X                       X.X  "
        " X.X                       X.X  "
        " X.X                       X.X  "
        " X.XXXXXX                  X.X  "
        "XX.XX...X                  X.X  "
        " X.XX...X             X X XX.X X"
        " X.XX...X                  X.X  "
        "XX.XX...X                  X.X  "
        " X.XX...X            X X XXX.X  "
        "    X...X                       "
        "    X...X               XX X  X "
        "XXXXX...XXXXX       XXX         "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor161 = Cursor(
        15, 17,
        "             XX    X            "
        "            X..X  XXX           "
        "            X..X  X..X          "
        "      XX    X...X X..X          "
        "     X..X   X...X X..X  XX      "
        "     X...X  X....XX...XX..X     "
        "     X....X  X...XX...XX...X    "
        "     XX...X  X...XX....XX..X    "
        "      X....X X....X....XX..X    "
        "      XX....XX....X....X...X    "
        "       X.....X....X....X...X    "
        "       XX....XX........X...X    "
        "        X.....X.............X   "
        "        XX..................X   "
        "         XX.................X   "
        "          X.................X   "
        "          XX................X   "
        "          XX................X   "
        "           X................X   "
        "           X................X   "
        "           X...............X    "
        "           X...............X    "
        "          X................X    "
        "          X................X    "
        "    XXXXXX.................X    "
        "   X........................X   "
        "   X.X......................X   "
        "   X..X.....................X   "
        "    XXXX....XXXX.............X  "
        "       XXXXXX  XX............X  "
        "                XX.........XX   "
        "                 XXXXXXXXXX     ");

    static Cursor cursor160 = Cursor(
        15, 17,
        "                                "
        "          XXXX  XXX             "
        "         X....XX...X            "
        "         X.....XX...XX          "
        "          X.....X....XXX        "
        "    XXX   X......X....X.X       "
        "   X...XX  XX....XX...X..X      "
        "   X.....XX XX...XX....X..X     "
        "   XX......X X....X....X..X     "
        "    XX......XX....X....X...X    "
        "     XX......X....X....X...X    "
        "      XX.....XX........X...X    "
        "        X.....X.............X   "
        "        XX..................X   "
        "         XX.................X   "
        "          X.................X   "
        "          XX................X   "
        "          XX................X   "
        "           X................X   "
        "           X................X   "
        "     XXX   X...............X    "
        "    X...X  X...............X    "
        "    X....XX................X    "
        "    X.X....................X    "
        "    X.X....................X    "
        "     X......................X   "
        "     X......................X   "
        "      XXX...................X   "
        "         XXXXXXX.............X  "
        "               XX............X  "
        "                XX.........XX   "
        "                 XXXXXXXXXX     ");

    static Cursor cursor163 = Cursor(
        6, 31,
        "                                "
        "                                "
        "                                "
        "        XXX           XXXXXXX   "
        "       XX.XX   XX     XXXXXXX   "
        "        X.X   X..X XXXX  X      "
        "       XXXXXXXXXXXXX..X  X      "
        "       X.XX..X..X..X..XXXXXX    "
        "       XXXX..X..X..X..X....X    "
        "       XXXX..X..X..X..X.XX.X    "
        "       XXXXXXXXXXXXXXXX....X    "
        "      XXXXXXXXXXXXXXXXX....X    "
        "       XXX.X.XX.X.XX.XXXXXXXX   "
        "      X.XX...XX...XX...XX.X     "
        "       XX XXX  XXX  XXX  X      "
        "                                "
        "                                "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor162 = Cursor(
        25, 31,
        "                                "
        "                                "
        "                                "
        "   XXXXXXX           XXX        "
        "   XXXXXXX     XX   XX.XX       "
        "      X  XXXX X..X   X.X        "
        "      X  X..XXXXXXXXXXXXX       "
        "    XXXXXX..X..X..X..XX.X       "
        "    X....X..X..X..X..XXXX       "
        "    X.XX.X..X..X..X..XXXX       "
        "    X....XXXXXXXXXXXXXXXX       "
        "    X....XXXXXXXXXXXXXXXXX      "
        "   XXXXXXXX.XX.X.XX.X.XXX       "
        "     X.XX...XX...XX...XX.X      "
        "      X  XXX  XXX  XXX XX       "
        "                                "
        "                                "
        "                       XXXXX    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                   XXXXX...XXXXX"
        "                    X.........X "
        "                     X.......X  "
        "                      X.....X   "
        "                       X...X    "
        "                        X.X     "
        "                         X      ");

    static Cursor cursor173 = Cursor(
        6, 31,
        "                                "
        "                                "
        "       XXXXXXXXXXXXXXXXXXXXX    "
        "      X.....................X   "
        "      X.XXX.XXX.XXX.XXX.XXX.X   "
        "      X.XXX.XXX.XXX.XXX.XXX.X   "
        "      X.XX..XX..XX..XX..XX..X   "
        "      X....................X    "
        "      X....................X    "
        "      XXXXX.XXX.XXX.XXX.XXX.X   "
        "      XXX.X.XXX.XXX.XXX.XXX.X   "
        "      XXX.X.XX..XX..XX..XX..X   "
        "      X.X.XXX..........XX...X   "
        "      X.X.X..X........X..X..X   "
        "       XXXX..XXXXXXXXXX..XXX    "
        "           XX          XX       "
        "                                "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor172 = Cursor(
        25, 31,
        "                                "
        "                                "
        "    XXXXXXXXXXXXXXXXXXXXX       "
        "   X.....................X      "
        "   X.XXX.XXX.XXX.XXX.XXX.X      "
        "   X.XXX.XXX.XXX.XXX.XXX.X      "
        "   X..XX..XX..XX..XX..XX.X      "
        "    X....................X      "
        "    X....................X      "
        "   X.XXX.XXX.XXX.XXX.XXXXX      "
        "   X.XXX.XXX.XXX.XXX.X.XXX      "
        "   X..XX..XX..XX..XX.X.XXX      "
        "   X...XX..........XXX.X.X      "
        "   X..X..X........X..X.X.X      "
        "    XXX..XXXXXXXXXX..XXXX       "
        "       XX          XX           "
        "                                "
        "                       XXXXX    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                   XXXXX...XXXXX"
        "                    X.........X "
        "                     X.......X  "
        "                      X.....X   "
        "                       X...X    "
        "                        X.X     "
        "                         X      ");

    static Cursor cursor178 = Cursor(
        6, 31,
        "                                "
        "                                "
        "               XXXXXXXXXXXXX    "
        "              X.............X   "
        "           X  X.XX..XX..XX..X   "
        "          XXXXX.X...X...X...X   "
        "         X....X.X...X...X...X   "
        "         XXX..X.X...X...X...X   "
        "      XXXXXX..X.X...X...X...X   "
        "     X........X.XX..XX..XX..X   "
        "     X........X.............X   "
        "     X..XX....XXXXXXXXXXXX.X    "
        "     X.XX.X..X...X  XX.XX X     "
        "     XXX.X.XXXXXXX  X.X.X X     "
        "       XX.X         XX.X        "
        "        XX           XX         "
        "                                "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor177 = Cursor(
        25, 31,
        "                                "
        "                                "
        "    XXXXXXXXXXXXX               "
        "   X.............X              "
        "   X..XX..XX..XX.X  X           "
        "   X...X...X...X.XXXXX          "
        "   X...X...X...X.X....X         "
        "   X...X...X...X.X..XXX         "
        "   X...X...X...X.X..XXXXXX      "
        "   X..XX..XX..XX.X........X     "
        "   X.............X........X     "
        "    X.XXXXXXXXXXXX....XX..X     "
        "     X XX.XX  X...X..X.XX.X     "
        "     X X.X.X  XXXXXXX.X.XXX     "
        "        X.XX         X.XX       "
        "         XX           XX        "
        "                                "
        "                       XXXXX    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                   XXXXX...XXXXX"
        "                    X.........X "
        "                     X.......X  "
        "                      X.....X   "
        "                       X...X    "
        "                        X.X     "
        "                         X      ");

    static Cursor cursor175 = Cursor(
        6, 31,
        "           XX                   "
        "          X..X                  "
        "         X.XX.X                 "
        "        X.X  X.X                "
        "         X.XX.X                 "
        "          X..X                  "
        "        XXXXXXXXXXXXXXXXXXX     "
        "       X...................X    "
        "       XXXXXXXXXXXXXXXXXXXXX    "
        "      XXX.X.XXX.XXX.XXX.X.XXX   "
        "      X.X.X.X.X.X.X.X.X.X.X.X   "
        "      XXX.X.XXX.XXX.XXX.X.XXX   "
        "      X...X.............X...X   "
        "      X...X.............X...X   "
        "       XXXXXXXXXXXXXXXXXXXXX    "
        "          XX XX     XX XX       "
        "                                "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor176 = Cursor(
        25, 31,
        "                   XX           "
        "                  X..X          "
        "                 X.XX.X         "
        "                X.X  X.X        "
        "                 X.XX.X         "
        "                  X..X          "
        "     XXXXXXXXXXXXXXXXXXX        "
        "    X...................X       "
        "    XXXXXXXXXXXXXXXXXXXXX       "
        "   XXX.X.XXX.XXX.XXX.X.XXX      "
        "   X.X.X.X.X.X.X.X.X.X.X.X      "
        "   XXX.X.XXX.XXX.XXX.X.XXX      "
        "   X...X.............X...X      "
        "   X...X.............X...X      "
        "    XXXXXXXXXXXXXXXXXXXXX       "
        "       XX XX     XX XX          "
        "                                "
        "                       XXXXX    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                       X...X    "
        "                   XXXXX...XXXXX"
        "                    X.........X "
        "                     X.......X  "
        "                      X.....X   "
        "                       X...X    "
        "                        X.X     "
        "                         X      ");

    static Cursor cursor180 = Cursor(
        6, 31,
        "                           XXXXX"
        "                          X..X.X"
        "                         X...X.X"
        "                        X...X.X "
        "                       X....X.X "
        "                      X.....X.X "
        "    XXXXXXXXXXXXXXXXXX.....X.X  "
        "   XXX.....................XXX  "
        " XX..........................X  "
        "XX.X.X.X.X.X.X.X.X.X.X.X....X   "
        "XX.........................X    "
        "XX........XXXXXXXX........X     "
        " XX......X........XX...XXX      "
        "   XXXXXX..XXXXXX...XXX         "
        "        XXX.X....XXX            "
        "          X.X.....XXX           "
        "          X.X....XX             "
        "    XXXXX  XXXXXX               "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor179 = Cursor(
        6, 31,
        "         X                      "
        "         X                      "
        "         X               X      "
        "         X               X      "
        "         X               X      "
        "         X    XXXX       X      "
        "        XXX   XXXX       X      "
        "       X...X  X..X       X      "
        "       X....XXXXXXXXXX   X      "
        "       X..............X  X      "
        "XXXXXXX......XX.XX.XX..XXXXXX   "
        "X............................X  "
        " XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "  XX....X..X.X..XXXX.XX..XXX.XXX"
        "   XXXXXXXXXXXXXXXXXXXXXXXXXXXX "
        "                                "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor164 = Cursor(
        14, 16,
        "                                "
        "                                "
        "                                "
        "                                "
        "                                "
        "              X                 "
        "             X.X                "
        "             X.X                "
        "           XXX.XXX              "
        "          X.......X             "
        "           X.....X              "
        "            X...X               "
        "       X     X.X     X          "
        "      X.X     X     X.X         "
        "      X..X         X..X         "
        "    XXX...X   X   X...XXX       "
        "   X.......X X.X X.......X      "
        "    XXX...X   X   X...XXX       "
        "      X..X         X..X         "
        "      X.X     X     X.X         "
        "       X     X.X     X          "
        "            X...X               "
        "           X.....X              "
        "          X.......X             "
        "           XXX.XXX              "
        "             X.X                "
        "             X.X                "
        "              X                 "
        "                                "
        "                                "
        "                                "
        "                                ");

    static Cursor cursor169 = Cursor(
        6, 31,
        "            XXXXXXXXXXXX        "
        "            X..........XX       "
        "            XXXXX.X.XX.X.X      "
        "            XXXXXXXXXXXX..X     "
        "            X..........XX..X    "
        "            X.XX.XX.XX.X.X.X    "
        "            X.X..X..X..X..XX    "
        "            X..........X.X.X    "
        "            X.XX.XX.XX.X.X.X    "
        "            X.X..XX.XX.X...X    "
        "     XXX    X..........X.X.X    "
        "    X...X   X.XX.XX.XX.XXXXXXX  "
        "   X.....X  X.X..XX.XX.X.....XX "
        "   X.XXX.X  X..........XXXXXXX.X"
        "XXXX.X...XXXX.XX.XX.XX.X.X.X.XXX"
        "X...........X.XX.X..XX.X.....X.X"
        "X.X.XXXXX.X.X..........X.X.X.X.X"
        "X...........X.XX.XX.XX.X.X.X.X.X"
        "X.X..XXX..X.X.X..XX.X..X.....X.X"
        "X....XX.....X..........X.X.X.X.X"
        "X.X..XX...X.X.XX.XX.XX.X.X.X.X.X"
        "X...XXXXX...X.X..XX.XX.X.....X.X"
        "X...X...XX..X....XX....X.X.X.X.X"
        "XXXXX...XXXXXXXXXXXXXXXXXXXXXXXX"
        "    X...XX                      "
        "XXXXX...XXXXX X XXXXXXXXXXXXXXX "
        " X.........XX                   "
        "  X.......XX     XX X XXXXX XX  "
        "   X.....XX                     "
        "    X...XX               XXXX   "
        "     X.XX                       "
        "      XX                        ");

    static Cursor cursor168 = Cursor(
        6, 31,
        "                                "
        "                                "
        "                                "
        "          XXXXXXXXXXXX          "
        "          X..........XX         "
        "          XXXXX.X.XX.X.X        "
        "          XXXXXXXXXXXX..X       "
        "          X..........XX..X      "
        "          X.XX.XX.XX.X.X.X      "
        "          X.X..X..X..X..XX      "
        "          X..........X.X.X      "
        "          X.XX.XX.XX.X.X.X      "
        "          X.X..XX.XX.X...X      "
        "          X..........X.X.X      "
        "          X.XX.XX.XX.X.X.X      "
        "          X.XX.X..XX.X...X      "
        "          X..........X.X.X      "
        "          X.XX.XX.XX.X.X.X      "
        "          X.X..XX.X..X...X      "
        "          X..........X.X.X      "
        "          X.XX.XX.XX.X.X.X      "
        "    XXXXX X.X..XX.XX.X...X      "
        "    X...X X....XX....X.X.X      "
        "    X...X XXXXXXXXXXXXXXXX      "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor170 = Cursor(
        6, 31,
        "                       XX   XX  "
        "                      XX.X XX.X "
        "                      XX.X XX.X "
        "                  XX  XXXX XXXX "
        "                 XX.X XX.X XX.X "
        "                 XX.X XXXX XXXX "
        "                 XXXX XX.X XX.X "
        "                 XX.X XX.X XX.X "
        "          XX     XXXX XXXX XX.X "
        "        XX..X    XX.X XX..XXX.X "
        "      XX..XX.X   XX.XX..XX.XX.X "
        "    XX..XXXXX.X  XXX..XXXXX.X.X "
        "  XX..XXX...XX.XXX..XXX...XX.XX "
        "  X.XXX....X.XX...XXX..X...XX.X "
        "  XXX...XX....XXXXX.XX...X..XXX "
        "  X...X........XX......X.....XX "
        "  X.XX.XX.XX.XX.X.XX.XX.XX.XX.X "
        "  X.XX.XX.XX.XX.X.XX.XX.XX.XX.X "
        "  X.XX.XX.XX.XX.X.XX.XX.XX.XX.X "
        "  X.XX.XX.XX.XX.X.XX.XX.XX.XX.X "
        "  XX.....X......X.........X...X "
        "  X.XXXXXX..X.XXX..XX...X....XX "
        "  X.X...XX......X.........XX..X "
        "  XXX...XXXXXXXXXXXXXXXXXXXXXXX "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor184 = Cursor(
        6, 31,
        "                 XXX            "
        "               XX...X           "
        "             XX....X            "
        "            X..XX.X             "
        "            X..X X    XXXXXXX   "
        "            XXX      X.......X  "
        " XX         X.X      X.XXX.X.X  "
        " X.X      XXX.XXXXXXXX.XXX.X.X  "
        "  X.X    X...........X.XXX.X.X  "
        "   X.X   X.XXXXXX....X.XXX.X.X  "
        "   X.X   X.XX.X......X.......X  "
        "    X.XXXXXXXXXXX..XXXX......X  "
        "    X.X..........X...........X  "
        "    X.X....XXXXXXXXXXX.XX.XXX   "
        "    X.XXXXX..X..X..X..X..X..XX  "
        "   X.X   X.XX.XX.XX.XX.XX.XX.X  "
        "   X.X   X.XX.XX.XX.XX.XX.XX.X  "
        " XX.X     X..X..X..X..X..X..X   "
        "X..X       XXXXXXXXXXXXXXXXX    "
        " XX XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor185 = Cursor(
        6, 31,
        "                X               "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               XXX              "
        "             XXXXXXXXXXXXXXX    "
        "            X...X.....XX..X     "
        "            X..XXX....XX.X      "
        "            X...X.....XX..X     "
        "             XXXXXXXXXXXXXXX    "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "               X.X              "
        "    XXXXX      X.X              "
        "    X...X      X.X              "
        "    X...X      X.X              "
        "    X...X      X.X              "
        "    X...X     XX.XX             "
        "    X...X     XXXXX             "
        "XXXXX...XXXXX XXX.X             "
        " X.........X  XXXXX             "
        "  X.......X   XXX.X             "
        "   X.....X    XXX.X             "
        "    X...X     XXXXX             "
        "     X.X       XXX              "
        "      X                         ");

    static Cursor cursor186 = Cursor(
        6, 31,
        "                 XXXXX          "
        "                X.....X         "
        "              XXX.....XXX       "
        "            XX..XX...XX..XX     "
        "           X..X..X...X..X..X    "
        "          X.X..XXX...XXX..X.X   "
        "         X...XXX  XXX  XXX...X  "
        "         X...XX         XX...X  "
        "        X.XX.X           X.XX.X "
        "        X...XX           XX...X "
        "        XXXXX             XXXXX "
        "        X...X             X...X "
        "       XXXXXXXX         XXXXXXXX"
        "       X......X         X......X"
        "        XXXXXX           XXXXXX "
        "        X...XXXXXXXXXXXXXXX...X "
        "        X...X X X X X X X X...X "
        "        X...X X X X X X X X...X "
        "        X...X X X X X X X X...X "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    static Cursor cursor189 = Cursor(
        6, 31,
        "                 XXXXXXX        "
        "              XXX.......XXX     "
        "            XX...XXXXXXX...XX   "
        "           X..XXXXXX.XXXXXX..X  "
        "          X.XXX.X X.X.X X.XXX.X "
        "         X.XX X.X  X.X  X.X XX.X"
        "         XXXXXXXXXXXXXXXXXXXXXXX"
        "         X.....................X"
        "         X.X.X.X.X.X.X.X.X.X.X.X"
        "         XXXXXXXXXXXXXXXXXXXXXXX"
        "         XXX                 XXX"
        "         X.X                 X.X"
        "         X.X                 X.X"
        "         X.X                 X.X"
        "         X.X                 X.X"
        "         X.X                 X.X"
        "         XXXXX             XXXXX"
        "         X....X           X....X"
        "         XXXXXX           XXXXXX"
        "    XXXXXX...X             X...X"
        "    X...XXXXXX             XXXXX"
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    // 0x00452001
    void initialiseCursors()
    {
        _cursors.push_back(SDL_CreateCursor(cursor116.data, cursor116.mask, 32, 32, cursor116.x, cursor116.y));
        _cursors.push_back(SDL_CreateCursor(cursor165.data, cursor165.mask, 32, 32, cursor165.x, cursor165.y));
        _cursors.push_back(SDL_CreateCursor(cursor109.data, cursor109.mask, 32, 32, cursor109.x, cursor109.y));
        _cursors.push_back(SDL_CreateCursor(cursor110.data, cursor110.mask, 32, 32, cursor110.x, cursor110.y));
        _cursors.push_back(SDL_CreateCursor(cursor112.data, cursor112.mask, 32, 32, cursor112.x, cursor112.y));
        _cursors.push_back(SDL_CreateCursor(cursor120.data, cursor120.mask, 32, 32, cursor120.x, cursor120.y));
        _cursors.push_back(SDL_CreateCursor(cursor119.data, cursor119.mask, 32, 32, cursor119.x, cursor119.y));
        _cursors.push_back(SDL_CreateCursor(cursor124.data, cursor124.mask, 32, 32, cursor124.x, cursor124.y));
        _cursors.push_back(SDL_CreateCursor(cursor131.data, cursor131.mask, 32, 32, cursor131.x, cursor131.y));
        _cursors.push_back(SDL_CreateCursor(cursor127.data, cursor127.mask, 32, 32, cursor127.x, cursor127.y));
        _cursors.push_back(SDL_CreateCursor(cursor128.data, cursor128.mask, 32, 32, cursor128.x, cursor128.y));
        _cursors.push_back(SDL_CreateCursor(cursor129.data, cursor129.mask, 32, 32, cursor129.x, cursor129.y));
        _cursors.push_back(SDL_CreateCursor(cursor130.data, cursor130.mask, 32, 32, cursor130.x, cursor130.y));
        _cursors.push_back(SDL_CreateCursor(cursor132.data, cursor132.mask, 32, 32, cursor132.x, cursor132.y));
        _cursors.push_back(SDL_CreateCursor(cursor133.data, cursor133.mask, 32, 32, cursor133.x, cursor133.y));
        _cursors.push_back(SDL_CreateCursor(cursor138.data, cursor138.mask, 32, 32, cursor138.x, cursor138.y));
        _cursors.push_back(SDL_CreateCursor(cursor137.data, cursor137.mask, 32, 32, cursor137.x, cursor137.y));
        _cursors.push_back(SDL_CreateCursor(cursor139.data, cursor139.mask, 32, 32, cursor139.x, cursor139.y));
        _cursors.push_back(SDL_CreateCursor(cursor141.data, cursor141.mask, 32, 32, cursor141.x, cursor141.y));
        _cursors.push_back(SDL_CreateCursor(cursor142.data, cursor142.mask, 32, 32, cursor142.x, cursor142.y));
        _cursors.push_back(SDL_CreateCursor(cursor143.data, cursor143.mask, 32, 32, cursor143.x, cursor143.y));
        _cursors.push_back(SDL_CreateCursor(cursor144.data, cursor144.mask, 32, 32, cursor144.x, cursor144.y));
        _cursors.push_back(SDL_CreateCursor(cursor145.data, cursor145.mask, 32, 32, cursor145.x, cursor145.y));
        _cursors.push_back(SDL_CreateCursor(cursor158.data, cursor158.mask, 32, 32, cursor158.x, cursor158.y));
        _cursors.push_back(SDL_CreateCursor(cursor159.data, cursor159.mask, 32, 32, cursor159.x, cursor159.y));
        _cursors.push_back(SDL_CreateCursor(cursor161.data, cursor161.mask, 32, 32, cursor161.x, cursor161.y));
        _cursors.push_back(SDL_CreateCursor(cursor160.data, cursor160.mask, 32, 32, cursor160.x, cursor160.y));
        _cursors.push_back(SDL_CreateCursor(cursor163.data, cursor163.mask, 32, 32, cursor163.x, cursor163.y));
        _cursors.push_back(SDL_CreateCursor(cursor162.data, cursor162.mask, 32, 32, cursor162.x, cursor162.y));
        _cursors.push_back(SDL_CreateCursor(cursor173.data, cursor173.mask, 32, 32, cursor173.x, cursor173.y));
        _cursors.push_back(SDL_CreateCursor(cursor172.data, cursor172.mask, 32, 32, cursor172.x, cursor172.y));
        _cursors.push_back(SDL_CreateCursor(cursor178.data, cursor178.mask, 32, 32, cursor178.x, cursor178.y));
        _cursors.push_back(SDL_CreateCursor(cursor177.data, cursor177.mask, 32, 32, cursor177.x, cursor177.y));
        _cursors.push_back(SDL_CreateCursor(cursor175.data, cursor175.mask, 32, 32, cursor175.x, cursor175.y));
        _cursors.push_back(SDL_CreateCursor(cursor176.data, cursor176.mask, 32, 32, cursor176.x, cursor176.y));
        _cursors.push_back(SDL_CreateCursor(cursor180.data, cursor180.mask, 32, 32, cursor180.x, cursor180.y));
        _cursors.push_back(SDL_CreateCursor(cursor179.data, cursor179.mask, 32, 32, cursor179.x, cursor179.y));
        _cursors.push_back(SDL_CreateCursor(cursor164.data, cursor164.mask, 32, 32, cursor164.x, cursor164.y));
        _cursors.push_back(SDL_CreateCursor(cursor169.data, cursor169.mask, 32, 32, cursor169.x, cursor169.y));
        _cursors.push_back(SDL_CreateCursor(cursor168.data, cursor168.mask, 32, 32, cursor168.x, cursor168.y));
        _cursors.push_back(SDL_CreateCursor(cursor170.data, cursor170.mask, 32, 32, cursor170.x, cursor170.y));
        _cursors.push_back(SDL_CreateCursor(cursor184.data, cursor184.mask, 32, 32, cursor184.x, cursor184.y));
        _cursors.push_back(SDL_CreateCursor(cursor185.data, cursor185.mask, 32, 32, cursor185.x, cursor185.y));
        _cursors.push_back(SDL_CreateCursor(cursor186.data, cursor186.mask, 32, 32, cursor186.x, cursor186.y));
        _cursors.push_back(SDL_CreateCursor(cursor189.data, cursor189.mask, 32, 32, cursor189.x, cursor189.y));
    }

    void disposeCursors()
    {
        for (auto cursor : _cursors)
        {
            SDL_FreeCursor(cursor);
        }
        _cursors.clear();
    }

    // 0x00407BA3
    // edx: cusor_id
    void setCursor(CursorId id)
    {
        if (_cursors.size() > 0)
        {
            auto index = (size_t)id;
            if (index >= _cursors.size())
            {
                // Default to cursor 0
                index = 0;
            }

            auto cursor = _cursors[index];
            if (cursor == nullptr && index != 0)
            {
                // Default to cursor 0
                cursor = _cursors[0];
            }
            SDL_SetCursor(cursor);
        }
    }

    // 0x00407FCD
    void getCursorPos(int32_t& x, int32_t& y)
    {
        SDL_GetMouseState(&x, &y);

        auto scale = Config::getNew().scale_factor;
        x /= scale;
        y /= scale;
    }

    // 0x00407FEE
    void setCursorPos(int32_t x, int32_t y)
    {
        auto scale = Config::getNew().scale_factor;
        x *= scale;
        y *= scale;

        SDL_WarpMouseInWindow(window, x, y);
    }

    void hideCursor()
    {
        SDL_ShowCursor(0);
    }

    void showCursor()
    {
        SDL_ShowCursor(1);
    }

    // 0x0040447F
    void initialiseInput()
    {
        call(0x0040447F);
    }

    // 0x004045C2
    void disposeInput()
    {
        call(0x004045C2);
    }

    // 0x004524C1
    void update()
    {
#ifdef _LOCO_WIN32_
        call(0x004524C1);
#endif
    }

    void update(int32_t width, int32_t height)
    {
        // Scale the width and height by configured scale factor
        auto scale_factor = Config::getNew().scale_factor;
        width = (int32_t)(width / scale_factor);
        height = (int32_t)(height / scale_factor);

        int32_t widthShift = 6;
        int16_t blockWidth = 1 << widthShift;
        int32_t heightShift = 3;
        int16_t blockHeight = 1 << heightShift;

        if (surface != nullptr)
        {
            SDL_FreeSurface(surface);
        }
        if (RGBASurface != nullptr)
        {
            SDL_FreeSurface(RGBASurface);
        }

        surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);

        RGBASurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
        SDL_SetSurfaceBlendMode(RGBASurface, SDL_BLENDMODE_NONE);

        SDL_SetSurfacePalette(surface, palette);

        int32_t pitch = surface->pitch;

        Gfx::Context context{};
        context.bits = new uint8_t[surface->pitch * height];
        context.width = width;
        context.height = height;
        context.pitch = pitch - width;

        screen_info->context = context;
        screen_info->width = width;
        screen_info->height = height;
        screen_info->width_2 = width;
        screen_info->height_2 = height;
        screen_info->width_3 = width;
        screen_info->height_3 = height;
        screen_info->dirty_block_width = blockWidth;
        screen_info->dirty_block_height = blockHeight;
        screen_info->dirty_block_columns = (width / blockWidth) + 1;
        screen_info->dirty_block_rows = (height / blockHeight) + 1;
        screen_info->dirty_block_column_shift = widthShift;
        screen_info->dirty_block_row_shift = heightShift;
        screen_info->dirty_blocks_initialised = 1;
    }

    static void positionChanged(int32_t x, int32_t y)
    {
        auto displayIndex = SDL_GetWindowDisplayIndex(window);

        auto& cfg = Config::getNew().display;
        if (cfg.index != displayIndex)
        {
            cfg.index = displayIndex;
            Config::writeNewConfig();
        }
    }

    void resize(int32_t width, int32_t height)
    {
        update(width, height);
        Gui::resize();
        Gfx::invalidateScreen();

        // Save window size to config if NOT maximized
        auto wf = SDL_GetWindowFlags(window);
        if (!(wf & SDL_WINDOW_MAXIMIZED) && !(wf & SDL_WINDOW_FULLSCREEN))
        {
            auto& cfg = Config::getNew().display;
            cfg.window_resolution = { width, height };
            Config::writeNewConfig();
        }

        auto options_window = WindowManager::find(WindowType::options);
        if (options_window != nullptr)
            options_window->moveToCentre();
    }

    void triggerResize()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        resize(width, height);
    }

    void render()
    {
        if (window == nullptr || surface == nullptr)
            return;

        if (!Ui::dirtyBlocksInitialised())
        {
            return;
        }

        WindowManager::updateViewports();

        if (!Intro::isActive())
        {
            Gfx::drawDirtyBlocks();
        }

        // Lock the surface before setting its pixels
        if (SDL_MUSTLOCK(surface))
        {
            if (SDL_LockSurface(surface) < 0)
            {
                return;
            }
        }

        // Draw FPS counter?
        if (Config::getNew().showFPS)
        {
            Drawing::drawFPS();
        }

        // Copy pixels from the virtual screen buffer to the surface
        auto& context = Gfx::screenContext();
        if (context.bits != nullptr)
        {
            std::memcpy(surface->pixels, context.bits, surface->pitch * surface->h);
        }

        // Unlock the surface
        if (SDL_MUSTLOCK(surface))
        {
            SDL_UnlockSurface(surface);
        }

        auto scale_factor = Config::getNew().scale_factor;
        if (scale_factor == 1 || scale_factor <= 0)
        {
            if (SDL_BlitSurface(surface, nullptr, SDL_GetWindowSurface(window), nullptr))
            {
                Console::error("SDL_BlitSurface %s", SDL_GetError());
                exit(1);
            }
        }
        else
        {
            // first blit to rgba surface to change the pixel format
            if (SDL_BlitSurface(surface, nullptr, RGBASurface, nullptr))
            {
                Console::error("SDL_BlitSurface %s", SDL_GetError());
                exit(1);
            }
            // then scale to window size. Without changing to RGBA first, SDL complains
            // about blit configurations being incompatible.
            if (SDL_BlitScaled(RGBASurface, nullptr, SDL_GetWindowSurface(window), nullptr))
            {
                Console::error("SDL_BlitScaled %s", SDL_GetError());
                exit(1);
            }
        }

        SDL_UpdateWindowSurface(window);
    }

    void updatePalette(const palette_entry_t* entries, int32_t index, int32_t count)
    {
        SDL_Color base[256];
        for (int i = 0; i < 256; i++)
        {
            auto& src = entries[i];
            base[i].r = src.r;
            base[i].g = src.g;
            base[i].b = src.b;
            base[i].a = 0;
        }
        SDL_SetPaletteColors(palette, base, 0, 256);
    }

    // 0x00406FBA
    static void enqueueKey(uint32_t keycode)
    {
        Input::enqueueKey(keycode);

        switch (keycode)
        {
            case SDLK_RETURN:
            case SDLK_BACKSPACE:
            case SDLK_DELETE:
            {
                char c[] = { (char)keycode, '\0' };
                Input::enqueueText(c);
                break;
            }
        }
    }

    static int32_t convertSdlScancodeToDirectInput(int32_t scancode)
    {
        switch (scancode)
        {
            case SDL_SCANCODE_UP: return DIK_UP;
            case SDL_SCANCODE_LEFT: return DIK_LEFT;
            case SDL_SCANCODE_RIGHT: return DIK_RIGHT;
            case SDL_SCANCODE_DOWN: return DIK_DOWN;
            case SDL_SCANCODE_LSHIFT: return DIK_LSHIFT;
            case SDL_SCANCODE_RSHIFT: return DIK_RSHIFT;
            case SDL_SCANCODE_LCTRL: return DIK_LCONTROL;
            case SDL_SCANCODE_RCTRL: return DIK_RCONTROL;
            case SDL_SCANCODE_INSERT:
                return DIK_INSERT;

            // Simulate INSERT for smaller keyboards
            case SDL_SCANCODE_LALT: return DIK_INSERT;
            case SDL_SCANCODE_RALT: return DIK_INSERT;

            default: return 0;
        }
    }

    static int32_t convertSdlKeycodeToWindows(int32_t keyCode)
    {
        switch (keyCode)
        {
            case SDLK_PAUSE: return VK_PAUSE;
            case SDLK_PAGEUP: return VK_PRIOR;
            case SDLK_PAGEDOWN: return VK_NEXT;
            case SDLK_END: return VK_END;
            case SDLK_HOME: return VK_HOME;
            case SDLK_LEFT: return VK_LEFT;
            case SDLK_UP: return VK_UP;
            case SDLK_RIGHT: return VK_RIGHT;
            case SDLK_DOWN: return VK_DOWN;
            case SDLK_SELECT: return VK_SELECT;
            case SDLK_EXECUTE: return VK_EXECUTE;
            case SDLK_PRINTSCREEN: return VK_SNAPSHOT;
            case SDLK_INSERT: return VK_INSERT;
            case SDLK_DELETE: return VK_DELETE;
            case SDLK_SEMICOLON: return VK_OEM_1;
            case SDLK_EQUALS: return VK_OEM_PLUS;
            case SDLK_COMMA: return VK_OEM_COMMA;
            case SDLK_MINUS: return VK_OEM_MINUS;
            case SDLK_PERIOD: return VK_OEM_PERIOD;
            case SDLK_SLASH: return VK_OEM_2;
            case SDLK_QUOTE: return VK_OEM_3;
            case SDLK_LEFTBRACKET: return VK_OEM_4;
            case SDLK_BACKSLASH: return VK_OEM_5;
            case SDLK_RIGHTBRACKET: return VK_OEM_6;
            case SDLK_HASH: return VK_OEM_7;
            case SDLK_BACKQUOTE: return VK_OEM_8;
            case SDLK_APPLICATION: return VK_APPS;
            case SDLK_KP_0: return VK_NUMPAD0;
            case SDLK_KP_1: return VK_NUMPAD1;
            case SDLK_KP_2: return VK_NUMPAD2;
            case SDLK_KP_3: return VK_NUMPAD3;
            case SDLK_KP_4: return VK_NUMPAD4;
            case SDLK_KP_5: return VK_NUMPAD5;
            case SDLK_KP_6: return VK_NUMPAD6;
            case SDLK_KP_7: return VK_NUMPAD7;
            case SDLK_KP_8: return VK_NUMPAD8;
            case SDLK_KP_9: return VK_NUMPAD9;
            case SDLK_KP_MULTIPLY: return VK_MULTIPLY;
            case SDLK_KP_PLUS: return VK_ADD;
            case SDLK_KP_ENTER: return VK_SEPARATOR;
            case SDLK_KP_MINUS: return VK_SUBTRACT;
            case SDLK_KP_PERIOD: return VK_DECIMAL;
            case SDLK_KP_DIVIDE: return VK_DIVIDE;
            default:
                if (keyCode >= SDLK_a && keyCode <= SDLK_z)
                {
                    return 'A' + (keyCode - SDLK_a);
                }
                else if (keyCode >= SDLK_F1 && keyCode <= SDLK_F12)
                {
                    return VK_F1 + (keyCode - SDLK_F1);
                }
                else if (keyCode <= 127)
                {
                    return keyCode;
                }
                return 0;
        }
    }

    // 0x0040477F
    static void readKeyboardState()
    {
        addr<0x005251CC, uint8_t>() = 0;
        auto dstSize = _keyboard_state.size();
        auto dst = _keyboard_state.get();

        int numKeys;

        std::fill_n(dst, dstSize, 0);
        auto keyboardState = SDL_GetKeyboardState(&numKeys);
        if (keyboardState != nullptr)
        {
            for (int scanCode = 0; scanCode < numKeys; scanCode++)
            {
                bool isDown = keyboardState[scanCode] != 0;
                if (!isDown)
                    continue;

                auto dinputCode = convertSdlScancodeToDirectInput(scanCode);
                if (dinputCode != 0)
                {
                    dst[dinputCode] = 0x80;
                }
            }
            addr<0x005251CC, uint8_t>() = 1;
        }
    }

    // 0x0040726D
    bool processMessages()
    {
#ifdef _LOCO_WIN32_
        return ((bool (*)())0x0040726D)();
#else
        using namespace Input;

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    return false;
                case SDL_WINDOWEVENT:
                    switch (e.window.event)
                    {
                        case SDL_WINDOWEVENT_MOVED:
                            positionChanged(e.window.data1, e.window.data2);
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            resize(e.window.data1, e.window.data2);
                            break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                {
                    auto scale_factor = Config::getNew().scale_factor;
                    auto x = (int32_t)(e.motion.x / scale_factor);
                    auto y = (int32_t)(e.motion.y / scale_factor);
                    auto xrel = (int32_t)(e.motion.xrel / scale_factor);
                    auto yrel = (int32_t)(e.motion.yrel / scale_factor);
                    Input::moveMouse(x, y, xrel, yrel);
                    break;
                }
                case SDL_MOUSEWHEEL:
                    addr<0x00525330, int32_t>() += e.wheel.y * 128;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                {
                    auto scale_factor = Config::getNew().scale_factor;
                    addr<0x0113E9D4, int32_t>() = (int32_t)(e.button.x / scale_factor);
                    addr<0x0113E9D8, int32_t>() = (int32_t)(e.button.y / scale_factor);
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            Input::enqueueMouseButton(1);
                            addr<0x0113E8A0, int32_t>() = 1;
                            break;
                        case SDL_BUTTON_RIGHT:
                            Input::enqueueMouseButton(2);
                            addr<0x0113E0C0, int32_t>() = 1;
                            addr<0x005251C8, int32_t>() = 1;
                            addr<0x01140845, uint8_t>() = 0x80;
                            break;
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    auto scale_factor = Config::getNew().scale_factor;
                    addr<0x0113E9D4, int32_t>() = (int32_t)(e.button.x / scale_factor);
                    addr<0x0113E9D8, int32_t>() = (int32_t)(e.button.y / scale_factor);
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            Input::enqueueMouseButton(3);
                            addr<0x0113E8A0, int32_t>() = 0;
                            break;
                        case SDL_BUTTON_RIGHT:
                            Input::enqueueMouseButton(4);
                            addr<0x0113E0C0, int32_t>() = 0;
                            addr<0x005251C8, int32_t>() = 0;
                            addr<0x01140845, uint8_t>() = 0;
                            break;
                    }
                    break;
                }
                case SDL_KEYDOWN:
                {
                    auto keycode = e.key.keysym.sym;

#if !(defined(__APPLE__) && defined(__MACH__))
                    // Toggle fullscreen when ALT+RETURN is pressed
                    if (keycode == SDLK_RETURN)
                    {
                        if ((e.key.keysym.mod & KMOD_LALT) || (e.key.keysym.mod & KMOD_RALT))
                        {
                            toggleFullscreenDesktop();
                        }
                    }
#endif

                    // Map keypad enter to normal enter
                    if (keycode == SDLK_KP_ENTER)
                    {
                        keycode = SDLK_RETURN;
                    }

                    auto locokey = convertSdlKeycodeToWindows(keycode);
                    if (locokey != 0)
                    {
                        enqueueKey(locokey);
                    }
                    break;
                }
                case SDL_KEYUP:
                    break;
                case SDL_TEXTINPUT:
                    enqueueText(e.text.text);
                    break;
            }
        }
        readKeyboardState();
        return true;
#endif
    }

    void showMessageBox(const std::string& title, const std::string& message)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, title.c_str(), message.c_str(), window);
    }

    static Config::Resolution getDisplayResolutionByMode(Config::ScreenMode mode)
    {
        auto& config = Config::getNew();
        if (mode == Config::ScreenMode::window)
        {
            if (config.display.window_resolution.isPositive())
                return config.display.window_resolution;
            else
                return { 800, 600 };
        }
        else if (mode == Config::ScreenMode::fullscreen && config.display.fullscreen_resolution.isPositive())
            return config.display.fullscreen_resolution;
        else
            return getDesktopResolution();
    }

    Config::Resolution getResolution()
    {
        return { Ui::width(), Ui::height() };
    }

    Config::Resolution getDesktopResolution()
    {
        int32_t displayIndex = SDL_GetWindowDisplayIndex(window);
        SDL_DisplayMode desktopDisplayMode;
        SDL_GetDesktopDisplayMode(displayIndex, &desktopDisplayMode);

        return { desktopDisplayMode.w, desktopDisplayMode.h };
    }

    bool setDisplayMode(Config::ScreenMode mode, Config::Resolution newResolution)
    {
        // First, set the appropriate screen mode flags.
        auto flags = 0;
        if (mode == Config::ScreenMode::fullscreen)
            flags |= SDL_WINDOW_FULLSCREEN;
        else if (mode == Config::ScreenMode::fullscreenBorderless)
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        // *HACK* Set window to non fullscreen before switching resolution.
        // This fixes issues with high dpi and Windows scaling affecting the gui size.
        SDL_SetWindowFullscreen(window, 0);

        // Set the new dimensions of the screen.
        if (mode == Config::ScreenMode::window)
        {
            auto desktopResolution = getDesktopResolution();
            auto x = (desktopResolution.width - newResolution.width) / 2;
            auto y = (desktopResolution.height - newResolution.height) / 2;
            SDL_SetWindowPosition(window, x, y);
        }
        SDL_SetWindowSize(window, newResolution.width, newResolution.height);

        // Set the window fullscreen mode.
        if (SDL_SetWindowFullscreen(window, flags) != 0)
        {
            Console::error("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
            return false;
        }

        // It appears we were successful in setting the screen mode, so let's up date the config.
        auto& config = Config::getNew();
        config.display.mode = mode;

        if (mode == Config::ScreenMode::window)
            config.display.window_resolution = newResolution;
        else if (mode == Config::ScreenMode::fullscreen)
            config.display.fullscreen_resolution = newResolution;

        // We're also keeping track the resolution in the legacy config, for now.
        auto& legacyConfig = Config::get();
        legacyConfig.resolution_width = newResolution.width;
        legacyConfig.resolution_height = newResolution.height;

        OpenLoco::Config::write();
        Ui::triggerResize();
        Gfx::invalidateScreen();

        return true;
    }

    bool setDisplayMode(Config::ScreenMode mode)
    {
        auto resolution = getDisplayResolutionByMode(mode);
        return setDisplayMode(mode, resolution);
    }

    void updateFullscreenResolutions()
    {
        // Query number of display modes
        int32_t displayIndex = SDL_GetWindowDisplayIndex(window);
        int32_t numDisplayModes = SDL_GetNumDisplayModes(displayIndex);

        // Get desktop aspect ratio
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(displayIndex, &mode);

        // Get resolutions
        auto resolutions = std::vector<Resolution>();
        float desktopAspectRatio = (float)mode.w / mode.h;
        for (int32_t i = 0; i < numDisplayModes; i++)
        {
            SDL_GetDisplayMode(displayIndex, i, &mode);
            if (mode.w > 0 && mode.h > 0)
            {
                float aspectRatio = (float)mode.w / mode.h;
                if (_resolutionsAllowAnyAspectRatio || std::fabs(desktopAspectRatio - aspectRatio) < 0.0001f)
                {
                    resolutions.push_back({ mode.w, mode.h });
                }
            }
        }

        // Sort by area
        std::sort(resolutions.begin(), resolutions.end(), [](const Resolution& a, const Resolution& b) -> bool {
            int32_t areaA = a.width * a.height;
            int32_t areaB = b.width * b.height;
            return areaA < areaB;
        });

        // Remove duplicates
        auto last = std::unique(resolutions.begin(), resolutions.end(), [](const Resolution& a, const Resolution& b) -> bool {
            return (a.width == b.width && a.height == b.height);
        });
        resolutions.erase(last, resolutions.end());

        // Update config fullscreen resolution if not set
        auto& cfg = Config::get();
        auto& cfg_new = Config::getNew();
        if (!(cfg_new.display.fullscreen_resolution.isPositive() && cfg.resolution_width > 0 && cfg.resolution_height > 0))
        {
            cfg.resolution_width = resolutions.back().width;
            cfg.resolution_height = resolutions.back().height;
            cfg_new.display.fullscreen_resolution.width = resolutions.back().width;
            cfg_new.display.fullscreen_resolution.height = resolutions.back().height;
        }

        _fsResolutions = resolutions;
    }

    std::vector<Resolution> getFullscreenResolutions()
    {
        updateFullscreenResolutions();
        return _fsResolutions;
    }

    Resolution getClosestResolution(int32_t inWidth, int32_t inHeight)
    {
        Resolution result = { 800, 600 };
        int32_t closestAreaDiff = -1;
        int32_t destinationArea = inWidth * inHeight;
        for (const Resolution& resolution : _fsResolutions)
        {
            // Check if exact match
            if (resolution.width == inWidth && resolution.height == inHeight)
            {
                result = resolution;
                break;
            }

            // Check if area is closer to best match
            int32_t areaDiff = std::abs((resolution.width * resolution.height) - destinationArea);
            if (closestAreaDiff == -1 || areaDiff < closestAreaDiff)
            {
                closestAreaDiff = areaDiff;
                result = resolution;
            }
        }
        return result;
    }

#if !(defined(__APPLE__) && defined(__MACH__))
    static void toggleFullscreenDesktop()
    {
        auto flags = SDL_GetWindowFlags(window);
        if (flags & SDL_WINDOW_FULLSCREEN)
        {
            setDisplayMode(Config::ScreenMode::window);
        }
        else
        {
            setDisplayMode(Config::ScreenMode::fullscreenBorderless);
        }
    }
#endif

    // 0x004C6EE6
    static Input::MouseButton gameGetNextInput(uint32_t* x, int16_t* y)
    {
        registers regs;
        call(0x004c6ee6, regs);

        *x = regs.eax;
        *y = regs.bx;

        return (Input::MouseButton)regs.cx;
    }

    // 0x004CD422
    static void processMouseTool(int16_t x, int16_t y)
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return;
        }

        auto toolWindow = WindowManager::find(_toolWindowType, _toolWindowNumber);
        if (toolWindow != nullptr)
        {
            toolWindow->callToolUpdate(_toolWidgetIdx, x, y);
        }
        else
        {
            Input::toolCancel();
        }
    }

    // 0x004C96E7
    void handleInput()
    {
        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_10))
        {
            Windows::CompanyWindow::openAndSetName();
        }

        bool set = _525E28 & (1 << 2);
        *_525E28 &= ~(1 << 2);
        if (set)
        {
            if (!isTitleMode() && !isEditorMode())
            {
                if (Tutorial::state() == Tutorial::State::none)
                {
                    call(0x4C95A6);
                }
            }
        }

        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_5))
        {
            GameCommands::do_21(2, 1);
        }

        if (!MultiPlayer::hasFlag(MultiPlayer::flags::flag_0) && !MultiPlayer::hasFlag(MultiPlayer::flags::flag_4))
        {
            if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_2))
            {
                WindowManager::closeConstructionWindows();
                WindowManager::closeAllFloatingWindows();
                GameCommands::do_69();
            }

            if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_3))
            {
                WindowManager::closeConstructionWindows();
                WindowManager::closeAllFloatingWindows();
                GameCommands::do_70();
            }
        }

        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_4))
        {
            GameCommands::do_72();
        }

        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_0))
        {
            WindowManager::closeConstructionWindows();
            WindowManager::closeAllFloatingWindows();
        }

        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_1))
        {
            GameCommands::do_21(0, 2);
        }

        if (Ui::dirtyBlocksInitialised())
        {
            WindowManager::callEvent8OnAllWindows();

            WindowManager::invalidateAllWindowsAfterInput();
            Input::updateCursorPosition();

            uint32_t x;
            int16_t y;
            Input::MouseButton state;
            while ((state = gameGetNextInput(&x, &y)) != Input::MouseButton::released)
            {
                if (isTitleMode() && Intro::isActive() && state == Input::MouseButton::leftPressed)
                {
                    if (Intro::state() == Intro::State::state_9)
                    {
                        Intro::state(Intro::State::end);
                        continue;
                    }
                    else
                    {
                        Intro::state(Intro::State::state_8);
                    }
                }
                Input::handleMouse(x, y, state);
            }

            if (Input::hasFlag(Input::Flags::flag5))
            {
                Input::handleMouse(x, y, state);
            }
            else if (x != 0x80000000)
            {
                x = std::clamp<int16_t>(x, 0, Ui::width() - 1);
                y = std::clamp<int16_t>(y, 0, Ui::height() - 1);

                Input::handleMouse(x, y, state);
                Input::processMouseOver(x, y);
                processMouseTool(x, y);
            }
        }

        WindowManager::callEvent9OnAllWindows();
    }

    // 0x004C98CF
    void minimalHandleInput()
    {
        WindowManager::callEvent8OnAllWindows();

        WindowManager::invalidateAllWindowsAfterInput();
        Input::updateCursorPosition();

        uint32_t x;
        int16_t y;
        Input::MouseButton state;
        while ((state = gameGetNextInput(&x, &y)) != Input::MouseButton::released)
        {
            Input::handleMouse(x, y, state);
        }

        if (Input::hasFlag(Input::Flags::flag5))
        {
            Input::handleMouse(x, y, state);
        }
        else if (x != 0x80000000)
        {
            x = std::clamp<int16_t>(x, 0, Ui::width() - 1);
            y = std::clamp<int16_t>(y, 0, Ui::height() - 1);

            Input::handleMouse(x, y, state);
            Input::processMouseOver(x, y);
            processMouseTool(x, y);
        }

        WindowManager::callEvent9OnAllWindows();
    }

    void setWindowScaling(float newScaleFactor)
    {
        auto& config = Config::getNew();
        newScaleFactor = std::clamp(newScaleFactor, ScaleFactor::min, ScaleFactor::max);
        if (config.scale_factor == newScaleFactor)
            return;

        config.scale_factor = newScaleFactor;

        OpenLoco::Config::write();
        Ui::triggerResize();
        Gfx::invalidateScreen();
    }

    void adjustWindowScale(float adjust_by)
    {
        auto& config = Config::getNew();
        float newScaleFactor = std::clamp(config.scale_factor + adjust_by, ScaleFactor::min, ScaleFactor::max);
        if (config.scale_factor == newScaleFactor)
            return;

        setWindowScaling(newScaleFactor);
    }
}
