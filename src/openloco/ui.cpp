#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

#ifdef _LOCO_WIN32_
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <SDL2/SDL.h>
    #pragma warning(disable : 4121) // alignment of a member was sensitive to packing
    #define NOMINMAX
    #include <SDL2/SDL_syswm.h>
    #pragma warning(default : 4121) // alignment of a member was sensitive to packing
#endif

#include "input.h"
#include "interop/interop.hpp"
#include "graphics/gfx.h"
#include "openloco.h"
#include "ui.h"
#include "windowmgr.h"

namespace openloco::ui
{
    constexpr auto WINDOW_CLASS_NAME = "Chris Sawyer's Locomotion";
    constexpr auto WINDOW_TITLE = "OpenLoco";

    #pragma pack(push, 1)

    struct palette_entry_t
    {
        uint8_t b, g, r, a;
    };

    struct screen_info_t
    {
        gfx::drawpixelinfo_t dpi;
        int16_t width;
        int16_t height;
        int16_t width_2;
        int16_t height_2;
        int16_t width_3;
        int16_t height_3;
        int16_t dirty_block_width;
        int16_t dirty_block_height;
        int32_t dirty_block_width_2;
        int32_t dirty_block_height_2;
        int8_t dirty_block_columns;
        int8_t dirty_block_rows;
        int8_t dirty_blocks_initialised;
    };

    #pragma pack(pop)

    using set_palette_func = void(*)(const palette_entry_t * palette, int32_t index, int32_t count);

    loco_global<void *, 0x00525320> hwnd;
    loco_global<screen_info_t, 0x0050B884> screen_info;
    loco_global<set_palette_func, 0x0052524C> set_palette_callback;
    loco_global_array<uint8_t, 256, 0x01140740> _keyboard_state;

    static SDL_Window * window;
    static SDL_Surface * surface;
    static SDL_Palette * palette;

    void update(int32_t width, int32_t height);
    void resize(int32_t width, int32_t height);

    int32_t width()
    {
        return screen_info->width;
    }

    int32_t height()
    {
        return screen_info->height;
    }

    void update_palette(const palette_entry_t * entries, int32_t index, int32_t count);

    // 0x00405409
    void create_window()
    {
#ifdef _LOCO_WIN32_
        hwnd = CreateWindowExA(
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
        int32_t initialWidth = 800;
        int32_t initialHeight = 600;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw std::runtime_error("Unable to initialise SDL2 video subsystem.");
        }

        // Create the window
        window = SDL_CreateWindow(
            "OpenLoco",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            initialWidth,
            initialHeight,
            SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            throw std::runtime_error("Unable to create SDL2 window.");
        }

        // Grab the HWND
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
        {
            throw std::runtime_error("Unable to fetch SDL2 window system handle.");
        }
        hwnd = wmInfo.info.win.window;

        // Create a palette for the window
        palette = SDL_AllocPalette(256);
        set_palette_callback = update_palette;

        update(initialWidth, initialHeight);
#endif
    }

    // 0x0045235D
    void initialise()
    {
#ifdef _LOCO_WIN32_
        LOCO_CALLPROC_X(0x0045235D);
#endif
    }

    // 0x00452001
    void initialise_cursors()
    {
        LOCO_CALLPROC_X(0x00452001);
    }

    // 0x0040447F
    void initialise_input()
    {
        LOCO_CALLPROC_X(0x0040447F);
    }

    // 0x004045C2
    void dispose_input()
    {
        LOCO_CALLPROC_X(0x004045C2);
    }

    // 0x004524C1
    void update()
    {
#ifdef _LOCO_WIN32_
        LOCO_CALLPROC_X(0x004524C1);
#endif
    }

    void update(int32_t width, int32_t height)
    {
        int32_t columns = 6;
        int32_t rows = 3;

        if (surface != nullptr)
        {
            SDL_FreeSurface(surface);
        }
        surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
        SDL_SetSurfacePalette(surface, palette);

        int32_t pitch = surface->pitch;

        gfx::drawpixelinfo_t dpi = { 0 };
        dpi.bits = new uint8_t[surface->pitch * height];
        dpi.width = width;
        dpi.height = height;
        dpi.pitch = pitch - width;

        screen_info->dpi = dpi;
        screen_info->width = width;
        screen_info->height = height;
        screen_info->width_2 = width;
        screen_info->height_2 = height;
        screen_info->width_3 = width;
        screen_info->height_3 = height;
        screen_info->dirty_block_width = 64;
        screen_info->dirty_block_height = 8;
        screen_info->dirty_block_width_2 = (width >> columns) + 1;
        screen_info->dirty_block_height_2 = (height >> rows) + 1;
        screen_info->dirty_block_columns = columns;
        screen_info->dirty_block_rows = rows;
        screen_info->dirty_blocks_initialised = 1;
    }

    void resize(int32_t width, int32_t height)
    {
        update(width, height);
        windowmgr::resize();
        gfx::invalidate_screen();
    }

    void render()
    {
        if (window != nullptr && surface != nullptr)
        {
            // Lock the surface before setting its pixels
            if (SDL_MUSTLOCK(surface))
            {
                if (SDL_LockSurface(surface) < 0)
                {
                    return;
                }
            }

            // Copy pixels from the virtual screen buffer to the surface
            gfx::drawpixelinfo_t &dpi = gfx::screen_dpi;
            if (dpi.bits != nullptr)
            {
                std::memcpy(surface->pixels, dpi.bits, surface->pitch * surface->h);
            }

            // Unlock the surface
            if (SDL_MUSTLOCK(surface))
            {
                SDL_UnlockSurface(surface);
            }

            // Copy the surface to the window
            SDL_BlitSurface(surface, nullptr, SDL_GetWindowSurface(window), nullptr);
            SDL_UpdateWindowSurface(window);
        }
    }

    void update_palette(const palette_entry_t * entries, int32_t index, int32_t count)
    {
        SDL_Color base[256];
        for (int i = 0; i < 256; i++)
        {
            auto &src = entries[i];
            auto dst = base[i];
            base[i].r = src.r;
            base[i].g = src.g;
            base[i].b = src.b;
            base[i].a = 0;
        }
        SDL_SetPaletteColors(palette, base, 0, 256);
    }

    // 0x00406FBA
    void enqueue_key(uint32_t keycode)
    {
        ((void(*)(uint32_t))(0x00406FBA))(keycode);
    }

    static int32_t convert_sdl_scancode_to_dinput(int32_t scancode)
    {
        switch (scancode)
        {
            case SDL_SCANCODE_UP: return 0xC8;
            case SDL_SCANCODE_LEFT: return 0xCB;
            case SDL_SCANCODE_RIGHT: return 0xCD;
            case SDL_SCANCODE_DOWN: return 0xD0;
            case SDL_SCANCODE_LSHIFT: return 0x2A;
            case SDL_SCANCODE_RSHIFT: return 0x36;
            case SDL_SCANCODE_LCTRL: return 0x1D;
            case SDL_SCANCODE_RCTRL: return 0x9D;
            default: return 0;
        }
    }

    // 0x0040477F
    static void read_keyboard_state()
    {
        auto dstSize = _keyboard_state.size();
        auto dst = _keyboard_state.get();

        int numKeys;
        auto keyboardState = SDL_GetKeyboardState(&numKeys);
        if (keyboardState != nullptr)
        {
            for (int scanCode = 0; scanCode < numKeys; scanCode++)
            {
                bool isDown = keyboardState[scanCode] != 0;
                auto dinputCode = convert_sdl_scancode_to_dinput(scanCode);
                if (dinputCode != 0)
                {
                    dst[dinputCode] = (isDown ? 0x80 : 0);
                }
            }
        }
        else
        {
            std::fill_n(dst, dstSize, 0);
        }
    }

    // 0x0040726D
    bool process_messages()
    {
#ifdef _LOCO_WIN32_
        return ((bool(*)())0x0040726D)();
#else
        using namespace input;

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    return false;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        resize(e.window.data1, e.window.data2);
                    }
                    break;
                case SDL_MOUSEMOTION:
                    LOCO_GLOBAL(0x0113E72C, int32_t) = e.motion.x;
                    LOCO_GLOBAL(0x0113E730, int32_t) = e.motion.y;
                    break;
                case SDL_MOUSEWHEEL:
                    LOCO_GLOBAL(0x00525330, int32_t) += e.wheel.y * 128;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    LOCO_GLOBAL(0x0113E9D4, int32_t) = e.button.x;
                    LOCO_GLOBAL(0x0113E9D8, int32_t) = e.button.y;
                    LOCO_GLOBAL(0x00525324, int32_t) = 1;
                    switch (e.button.button) {
                    case SDL_BUTTON_LEFT:
                        input::enqueue_mouse_button(mouse_button::left_down);
                        LOCO_GLOBAL(0x0113E8A0, int32_t) = 1;
                        break;
                    case SDL_BUTTON_RIGHT:
                        input::enqueue_mouse_button(mouse_button::right_down);
                        LOCO_GLOBAL(0x0113E0C0, int32_t) = 1;
                        break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    LOCO_GLOBAL(0x0113E9D4, int32_t) = e.button.x;
                    LOCO_GLOBAL(0x0113E9D8, int32_t) = e.button.y;
                    LOCO_GLOBAL(0x00525324, int32_t) = 1;
                    switch (e.button.button) {
                    case SDL_BUTTON_LEFT:
                        input::enqueue_mouse_button(mouse_button::left_up);
                        LOCO_GLOBAL(0x0113E8A0, int32_t) = 0;
                        break;
                    case SDL_BUTTON_RIGHT:
                        input::enqueue_mouse_button(mouse_button::right_up);
                        LOCO_GLOBAL(0x0113E0C0, int32_t) = 0;
                        break;
                    }
                    break;
                case SDL_KEYDOWN:
                {
                    auto keycode = e.key.keysym.sym;
                    if (!(keycode & SDLK_SCANCODE_MASK))
                    {
                        // TODO convert to Microsoft VK codes
                        //      https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
                        enqueue_key(keycode);
                    }
                    break;
                }
                case SDL_KEYUP:
                    break;
            }
        }
        read_keyboard_state();
        return true;
#endif
    }
}
