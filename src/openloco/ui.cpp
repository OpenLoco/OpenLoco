#include "win32.h"
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
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

#include "config.h"
#include "console.h"
#include "environment.h"
#include "graphics/gfx.h"
#include "gui.h"
#include "input.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include "ui.h"
#include "utility/string.hpp"
#include "windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui
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
    loco_global<screen_info_t, 0x0050B884> screen_info;
    loco_global<set_palette_func, 0x0052524C> set_palette_callback;
    loco_global<uint8_t[256], 0x01140740> _keyboard_state;

    bool _resolutionsAllowAnyAspectRatio = false;
    std::vector<Resolution> _fsResolutions;

    static SDL_Window* window;
    static SDL_Surface* surface;
    static SDL_Palette* palette;
    static std::vector<SDL_Cursor*> _cursors;

    static void update(int32_t width, int32_t height);
    static void resize(int32_t width, int32_t height);
    static int32_t convert_sdl_keycode_to_windows(int32_t keyCode);
#if !(defined(__APPLE__) && defined(__MACH__))
    static void toggle_fullscreen_desktop();
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

    void update_palette(const palette_entry_t* entries, int32_t index, int32_t count);

    static sdl_window_desc get_window_desc(const config::display_config& cfg)
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
            case config::screen_mode::window:
                break;
            case config::screen_mode::fullscreen:
                desc.width = cfg.fullscreen_resolution.width;
                desc.height = cfg.fullscreen_resolution.height;
                desc.flags |= SDL_WINDOW_FULLSCREEN;
                break;
            case config::screen_mode::fullscreen_borderless:
                desc.flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                break;
        }
#endif
        return desc;
    }

    // 0x00405409
    void create_window(const config::display_config& cfg)
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
        auto desc = get_window_desc(cfg);
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

        // Create a palette for the window
        palette = SDL_AllocPalette(256);
        set_palette_callback = update_palette;

        update(desc.width, desc.height);
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

    // 0x00452001
    void initialise_cursors()
    {
        _cursors.push_back(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
        _cursors.push_back(nullptr);
        _cursors.push_back(nullptr);
        _cursors.push_back(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS));
        _cursors.push_back(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND));
        _cursors.push_back(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT));
        _cursors.push_back(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE));
    }

    void dispose_cursors()
    {
        for (auto cursor : _cursors)
        {
            SDL_FreeCursor(cursor);
        }
        _cursors.clear();
    }

    // 0x00407BA3
    // edx: cusor_id
    void set_cursor(cursor_id id)
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
    void get_cursor_pos(int32_t& x, int32_t& y)
    {
        SDL_GetMouseState(&x, &y);
    }

    // 0x00407FEE
    void set_cursor_pos(int32_t x, int32_t y)
    {
        SDL_WarpMouseInWindow(window, x, y);
    }

    void hide_cursor()
    {
        SDL_ShowCursor(0);
    }

    void show_cursor()
    {
        SDL_ShowCursor(1);
    }

    // 0x0040447F
    void initialise_input()
    {
        call(0x0040447F);
    }

    // 0x004045C2
    void dispose_input()
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

    static void position_changed(int32_t x, int32_t y)
    {
        auto displayIndex = SDL_GetWindowDisplayIndex(window);

        auto& cfg = config::get_new().display;
        if (cfg.index != displayIndex)
        {
            cfg.index = displayIndex;
            config::write_new_config(g_ctx.get<environment>());
        }
    }

    void resize(int32_t width, int32_t height)
    {
        update(width, height);
        gui::resize();
        gfx::invalidate_screen();

        // Save window size to config if NOT maximized
        auto wf = SDL_GetWindowFlags(window);
        if (!(wf & SDL_WINDOW_MAXIMIZED) && !(wf & SDL_WINDOW_FULLSCREEN))
        {
            auto& cfg = config::get_new().display;
            cfg.window_resolution = { width, height };
            config::write_new_config(g_ctx.get<environment>());
        }
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
            auto& dpi = gfx::screen_dpi();
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

    void update_palette(const palette_entry_t* entries, int32_t index, int32_t count)
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

    static void enqueue_text(const char* text)
    {
        if (text != nullptr && text[0] != '\0')
        {
#pragma pack(push, 1)
            struct key_queue_item_t
            {
                uint32_t a;
                uint32_t b;
            };
#pragma pack(pop)
            auto queue = (key_queue_item_t*)0x0113E300;
            auto index = addr<0x00525388, uint32_t>();
            queue[index].b = text[0];
        }
    }

    // 0x00406FBA
    static void enqueue_key(uint32_t keycode)
    {
        ((void (*)(uint32_t))(0x00406FBA))(keycode);

        switch (keycode)
        {
            case SDLK_RETURN:
            case SDLK_BACKSPACE:
            case SDLK_DELETE:
            {
                char c[] = { (char)keycode, '\0' };
                enqueue_text(c);
                break;
            }
        }
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

    static int32_t convert_sdl_keycode_to_windows(int32_t keyCode)
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
        return ((bool (*)())0x0040726D)();
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
                    switch (e.window.event)
                    {
                        case SDL_WINDOWEVENT_MOVED:
                            position_changed(e.window.data1, e.window.data2);
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            resize(e.window.data1, e.window.data2);
                            break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    input::move_mouse(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
                    break;
                case SDL_MOUSEWHEEL:
                    addr<0x00525330, int32_t>() += e.wheel.y * 128;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    addr<0x0113E9D4, int32_t>() = e.button.x;
                    addr<0x0113E9D8, int32_t>() = e.button.y;
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            input::enqueue_mouse_button(mouse_button::left_pressed);
                            addr<0x0113E8A0, int32_t>() = 1;
                            break;
                        case SDL_BUTTON_RIGHT:
                            input::enqueue_mouse_button(mouse_button::left_released);
                            addr<0x0113E0C0, int32_t>() = 1;
                            addr<0x005251C8, int32_t>() = 1;
                            addr<0x01140845, uint8_t>() = 0x80;
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    addr<0x0113E9D4, int32_t>() = e.button.x;
                    addr<0x0113E9D8, int32_t>() = e.button.y;
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            input::enqueue_mouse_button(mouse_button::right_pressed);
                            addr<0x0113E8A0, int32_t>() = 0;
                            break;
                        case SDL_BUTTON_RIGHT:
                            input::enqueue_mouse_button(mouse_button::right_released);
                            addr<0x0113E0C0, int32_t>() = 0;
                            addr<0x005251C8, int32_t>() = 0;
                            addr<0x01140845, uint8_t>() = 0;
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                {
                    auto keycode = e.key.keysym.sym;

#if !(defined(__APPLE__) && defined(__MACH__))
                    // Toggle fullscreen when ALT+RETURN is pressed
                    if (keycode == SDLK_RETURN)
                    {
                        if ((e.key.keysym.mod & KMOD_LALT) || (e.key.keysym.mod & KMOD_RALT))
                        {
                            toggle_fullscreen_desktop();
                        }
                    }
#endif

                    auto locokey = convert_sdl_keycode_to_windows(keycode);
                    if (locokey != 0)
                    {
                        enqueue_key(locokey);
                    }
                    break;
                }
                case SDL_KEYUP:
                    break;
                case SDL_TEXTINPUT:
                    enqueue_text(e.text.text);
                    break;
            }
        }
        read_keyboard_state();
        return true;
#endif
    }

    void show_message_box(const std::string& title, const std::string& message)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, title.c_str(), message.c_str(), window);
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
        auto& cfg = config::get();
        if (cfg.resolution_width == std::numeric_limits<uint16_t>::max() && cfg.resolution_height == std::numeric_limits<uint16_t>::max())
        {
            cfg.resolution_width = resolutions.back().width;
            cfg.resolution_height = resolutions.back().height;
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
    void set_screen_mode(config::screen_mode mode)
    {
        auto flags = 0;
        switch (mode)
        {
            case config::screen_mode::window:
                break;
            case config::screen_mode::fullscreen:
                flags |= SDL_WINDOW_FULLSCREEN;
                break;
            case config::screen_mode::fullscreen_borderless:
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                break;
        }

        if (SDL_SetWindowFullscreen(window, flags) != 0)
        {
            console::error("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
        }
        else
        {
            auto& cfg = config::get_new();
            cfg.display.mode = mode;
            config::write_new_config(g_ctx.get<environment>());
        }
    }

    static void toggle_fullscreen_desktop()
    {
        auto flags = SDL_GetWindowFlags(window);
        if (flags & SDL_WINDOW_FULLSCREEN)
        {
            set_screen_mode(config::screen_mode::window);
        }
        else
        {
            set_screen_mode(config::screen_mode::fullscreen_borderless);
        }
    }
#endif
}
