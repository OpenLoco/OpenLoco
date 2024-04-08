#include "GameStateFlags.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Ui/Cursor.h"
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>

#ifdef _WIN32
#include <OpenLoco/Resources/Resource.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

// `small` is used as a type in `windows.h`
#undef small
#endif

#include <SDL2/SDL.h>
#pragma warning(disable : 4121) // alignment of a member was sensitive to packing
#include <SDL2/SDL_syswm.h>
#pragma warning(default : 4121) // alignment of a member was sensitive to packing

#include "Config.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "Graphics/FPSCounter.h"
#include "Graphics/Gfx.h"
#include "Gui.h"
#include "Input.h"
#include "Intro.h"
#include "Logging.h"
#include "MultiPlayer.h"
#include "SceneManager.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/ToolManager.h"
#include "Ui/WindowManager.h"
#include "Window.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/String.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Ui
{
#pragma pack(push, 1)
    struct sdl_window_desc
    {
        int32_t x{};
        int32_t y{};
        int32_t width{};
        int32_t height{};
        int32_t flags{};
    };
#pragma pack(pop)

#ifdef _WIN32
    loco_global<void*, 0x00525320> _hwnd;
#endif // _WIN32
    // TODO: Move this into renderer.
    static loco_global<ScreenInfo, 0x0050B894> _screenInfo;
    loco_global<uint8_t[256], 0x01140740> _keyboardState;

    bool _resolutionsAllowAnyAspectRatio = false;
    std::vector<Resolution> _fsResolutions;

    static SDL_Window* window;
    static std::map<CursorId, SDL_Cursor*> _cursors;
    static CursorId _currentCursor = CursorId::pointer;
    static bool _exitRequested = false;

    static void setWindowIcon();
    static void resize(int32_t width, int32_t height);
    static Config::Resolution getDisplayResolutionByMode(Config::ScreenMode mode);

#if !(defined(__APPLE__) && defined(__MACH__))
    static void toggleFullscreenDesktop();
#endif

#ifdef _WIN32
    void* hwnd()
    {
        return _hwnd;
    }
#else
    void* hwnd()
    {
        return nullptr;
    }
#endif // _WIN32

    int32_t width()
    {
        return _screenInfo->width;
    }

    int32_t height()
    {
        return _screenInfo->height;
    }

    // TODO: Rename misleading name.
    bool dirtyBlocksInitialised()
    {
        return Gfx::getDrawingEngine().isInitialized();
    }

    static sdl_window_desc getWindowDesc(const Config::Display& cfg)
    {
        sdl_window_desc desc;
        desc.x = SDL_WINDOWPOS_CENTERED_DISPLAY(cfg.index);
        desc.y = SDL_WINDOWPOS_CENTERED_DISPLAY(cfg.index);
        desc.width = std::max(640, cfg.windowResolution.width);
        desc.height = std::max(480, cfg.windowResolution.height);
        desc.flags = SDL_WINDOW_RESIZABLE;
#if !(defined(__APPLE__) && defined(__MACH__))
        switch (cfg.mode)
        {
            case Config::ScreenMode::window:
                break;
            case Config::ScreenMode::fullscreen:
                desc.width = cfg.fullscreenResolution.width;
                desc.height = cfg.fullscreenResolution.height;
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
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw Exception::RuntimeError("Unable to initialise SDL2 video subsystem.");
        }

        // Create the window
        auto desc = getWindowDesc(cfg);
        window = SDL_CreateWindow("OpenLoco", desc.x, desc.y, desc.width, desc.height, desc.flags);
        if (window == nullptr)
        {
            throw Exception::RuntimeError("Unable to create SDL2 window.");
        }

#ifdef _WIN32
        // Grab the HWND
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
        {
            throw Exception::RuntimeError("Unable to fetch SDL2 window system handle.");
        }
        _hwnd = wmInfo.info.win.window;
#endif

        setWindowIcon();

        // Create a palette for the window
        auto& drawingEngine = Gfx::getDrawingEngine();
        drawingEngine.initialize(window);
        drawingEngine.resize(desc.width, desc.height);
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
        SDL_RestoreWindow(window);
    }

    static SDL_Cursor* loadCursor(Cursor& cursor)
    {
        return SDL_CreateCursor(cursor.data, cursor.mask, 32, 32, cursor.x, cursor.y);
    }

    // 0x00452001
    void initialiseCursors()
    {
        _cursors[CursorId::pointer] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        _cursors[CursorId::blank] = loadCursor(Cursor::blank);
        _cursors[CursorId::upArrow] = loadCursor(Cursor::upArrow);
        _cursors[CursorId::upDownArrow] = loadCursor(Cursor::upDownArrow);
        _cursors[CursorId::handPointer] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
        _cursors[CursorId::busy] = loadCursor(Cursor::busy);
        _cursors[CursorId::diagonalArrows] = loadCursor(Cursor::diagonalArrows);
        _cursors[CursorId::picker] = loadCursor(Cursor::picker);
        _cursors[CursorId::plantTree] = loadCursor(Cursor::plantTree);
        _cursors[CursorId::placeFountain] = loadCursor(Cursor::placeFountain);
        _cursors[CursorId::placeStatue] = loadCursor(Cursor::placeStatue);
        _cursors[CursorId::placeBench] = loadCursor(Cursor::placeBench);
        _cursors[CursorId::crosshair] = loadCursor(Cursor::crosshair);
        _cursors[CursorId::placeTrashBin] = loadCursor(Cursor::placeTrashBin);
        _cursors[CursorId::placeLantern] = loadCursor(Cursor::placeLantern);
        _cursors[CursorId::placeFence] = loadCursor(Cursor::placeFence);
        _cursors[CursorId::placeFlowers] = loadCursor(Cursor::placeFlowers);
        _cursors[CursorId::placePath] = loadCursor(Cursor::placePath);
        _cursors[CursorId::landTool] = loadCursor(Cursor::landTool);
        _cursors[CursorId::waterTool] = loadCursor(Cursor::waterTool);
        _cursors[CursorId::placeHome] = loadCursor(Cursor::placeHome);
        _cursors[CursorId::placeVolcano] = loadCursor(Cursor::placeVolcano);
        _cursors[CursorId::footsteps] = loadCursor(Cursor::footsteps);
        _cursors[CursorId::brush] = loadCursor(Cursor::brush);
        _cursors[CursorId::placeBanner] = loadCursor(Cursor::placeBanner);
        _cursors[CursorId::openHand] = loadCursor(Cursor::openHand);
        _cursors[CursorId::dragHand] = loadCursor(Cursor::dragHand);
        _cursors[CursorId::placeTrain] = loadCursor(Cursor::placeTrain);
        _cursors[CursorId::placeTrainAlt] = loadCursor(Cursor::placeTrainAlt);
        _cursors[CursorId::placeBus] = loadCursor(Cursor::placeBus);
        _cursors[CursorId::placeBusAlt] = loadCursor(Cursor::placeBusAlt);
        _cursors[CursorId::placeTruck] = loadCursor(Cursor::placeTruck);
        _cursors[CursorId::placeTruckAlt] = loadCursor(Cursor::placeTruckAlt);
        _cursors[CursorId::placeTram] = loadCursor(Cursor::placeTram);
        _cursors[CursorId::placeTramAlt] = loadCursor(Cursor::placeTramAlt);
        _cursors[CursorId::placePlane] = loadCursor(Cursor::placePlane);
        _cursors[CursorId::placeShip] = loadCursor(Cursor::placeShip);
        _cursors[CursorId::inwardArrows] = loadCursor(Cursor::inwardArrows);
        _cursors[CursorId::placeTown] = loadCursor(Cursor::placeTown);
        _cursors[CursorId::placeBuilding] = loadCursor(Cursor::placeBuilding);
        _cursors[CursorId::placeFactory] = loadCursor(Cursor::placeFactory);
        _cursors[CursorId::bulldozerTool] = loadCursor(Cursor::bulldozerTool);
        _cursors[CursorId::placeSignal] = loadCursor(Cursor::placeSignal);
        _cursors[CursorId::placeHQ] = loadCursor(Cursor::placeHQ);
        _cursors[CursorId::placeStation] = loadCursor(Cursor::placeStation);
    }

    void disposeCursors()
    {
        for (auto cursor : _cursors)
        {
            SDL_FreeCursor(cursor.second);
        }
        _cursors.clear();
    }

    // 0x00407BA3
    // edx: cusor_id
    void setCursor(CursorId id)
    {
        if (_cursors.size() > 0)
        {
            if (_cursors.find(id) == _cursors.end())
            {
                // Default to cursor 0
                id = CursorId::pointer;
            }

            _currentCursor = id;
            SDL_SetCursor(_cursors[id]);
        }
    }

    CursorId getCursor()
    {
        return _currentCursor;
    }

    // 0x00407FCD
    Point32 getCursorPosScaled()
    {
        auto unscaledPos = getCursorPos();

        auto scale = Config::get().scaleFactor;

        auto x = unscaledPos.x / scale;
        auto y = unscaledPos.y / scale;
        return { static_cast<int32_t>(std::round(x)), static_cast<int32_t>(std::round(y)) };
    }

    Point32 getCursorPos()
    {
        int x = 0, y = 0;
        SDL_GetMouseState(&x, &y);
        return { x, y };
    }

    // 0x00407FEE
    void setCursorPosScaled(int32_t scaledX, int32_t scaledY)
    {
        auto scale = Config::get().scaleFactor;
        auto unscaledX = scaledX * scale;
        auto unscaledY = scaledY * scale;
        setCursorPos(unscaledX, unscaledY);
    }

    void setCursorPos(int32_t x, int32_t y)
    {
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
    }

    static void positionChanged([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y)
    {
        auto displayIndex = SDL_GetWindowDisplayIndex(window);

        auto& cfg = Config::get().display;
        if (cfg.index != displayIndex)
        {
            cfg.index = displayIndex;
            Config::write();
        }
    }

    void resize(int32_t width, int32_t height)
    {
        auto& drawingEngine = Gfx::getDrawingEngine();
        drawingEngine.resize(width, height);

        Gui::resize();
        Gfx::invalidateScreen();

        // Save window size to config if NOT maximized
        auto wf = SDL_GetWindowFlags(window);
        if (!(wf & SDL_WINDOW_MAXIMIZED) && !(wf & SDL_WINDOW_FULLSCREEN))
        {
            auto& cfg = Config::get().display;
            cfg.windowResolution = { width, height };
            Config::write();
        }

        auto optionsWindow = WindowManager::find(WindowType::options);
        if (optionsWindow != nullptr)
            optionsWindow->moveToCentre();
    }

    void triggerResize()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        resize(width, height);
    }

    void render()
    {
        if (window == nullptr)
            return;

        auto& drawingEngine = Gfx::getDrawingEngine();

        if (!Ui::dirtyBlocksInitialised())
        {
            return;
        }

        WindowManager::updateViewports();

        if (!Intro::isActive())
        {
            drawingEngine.render();
        }

        // Draw FPS counter?
        if (Config::get().showFPS)
        {
            Gfx::drawFPS();
        }

        drawingEngine.present();
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

    // 0x0040477F
    static void readKeyboardState()
    {
        addr<0x005251CC, uint8_t>() = 0;
        auto dstSize = _keyboardState.size();
        auto dst = _keyboardState.get();

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

                dst[scanCode] = 0x80;
            }
            addr<0x005251CC, uint8_t>() = 1;
        }
    }

    // 0x004072EC
    bool processMessagesMini()
    {
        using namespace Input;

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    return false;
            }
        }
        return false;
    }

    // 0x0040726D
    bool processMessages()
    {
        using namespace Input;

        // The game has more than one loop for processing messages, if the secondary loop receives
        // SDL_QUIT then the message would be lost for the primary loop so we have to keep track of it.
        if (_exitRequested)
        {
            return false;
        }

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    _exitRequested = true;
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
                    auto scaleFactor = Config::get().scaleFactor;
                    auto x = static_cast<int32_t>(e.motion.x / scaleFactor);
                    auto y = static_cast<int32_t>(e.motion.y / scaleFactor);
                    auto xrel = static_cast<int32_t>(e.motion.xrel / scaleFactor);
                    auto yrel = static_cast<int32_t>(e.motion.yrel / scaleFactor);
                    Input::moveMouse(x, y, xrel, yrel);
                    break;
                }
                case SDL_MOUSEWHEEL:
                    Input::mouseWheel(e.wheel.y);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                {
                    auto scaleFactor = Config::get().scaleFactor;
                    const auto x = static_cast<int32_t>(e.button.x / scaleFactor);
                    const auto y = static_cast<int32_t>(e.button.y / scaleFactor);
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            Input::enqueueMouseButton({ { x, y }, 1 });
                            addr<0x0113E8A0, int32_t>() = 1;
                            break;
                        case SDL_BUTTON_RIGHT:
                            Input::enqueueMouseButton({ { x, y }, 2 });
                            addr<0x0113E0C0, int32_t>() = 1;
                            setRightMouseButtonDown(true);
                            addr<0x01140845, uint8_t>() = 0x80;
                            break;
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    auto scaleFactor = Config::get().scaleFactor;
                    const auto x = static_cast<int32_t>(e.button.x / scaleFactor);
                    const auto y = static_cast<int32_t>(e.button.y / scaleFactor);
                    addr<0x00525324, int32_t>() = 1;
                    switch (e.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            Input::enqueueMouseButton({ { x, y }, 3 });
                            addr<0x0113E8A0, int32_t>() = 0;
                            break;
                        case SDL_BUTTON_RIGHT:
                            Input::enqueueMouseButton({ { x, y }, 4 });
                            addr<0x0113E0C0, int32_t>() = 0;
                            setRightMouseButtonDown(false);
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

                    enqueueKey(keycode);
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
    }

    void showMessageBox(const std::string& title, const std::string& message)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, title.c_str(), message.c_str(), window);
    }

    static Config::Resolution getDisplayResolutionByMode(Config::ScreenMode mode)
    {
        auto& config = Config::get();
        if (mode == Config::ScreenMode::window)
        {
            if (config.display.windowResolution.isPositive())
                return config.display.windowResolution;
            else
                return { 800, 600 };
        }
        else if (mode == Config::ScreenMode::fullscreen && config.display.fullscreenResolution.isPositive())
            return config.display.fullscreenResolution;
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
            Logging::error("SDL_SetWindowFullscreen failed: {}", SDL_GetError());
            return false;
        }

        // It appears we were successful in setting the screen mode, so let's up date the config.
        auto& config = Config::get();
        config.display.mode = mode;

        if (mode == Config::ScreenMode::window)
            config.display.windowResolution = newResolution;
        else if (mode == Config::ScreenMode::fullscreen)
            config.display.fullscreenResolution = newResolution;

        // We're also keeping track the resolution in the legacy config, for now.
        auto& legacyConfig = Config::get().old;
        legacyConfig.resolutionWidth = newResolution.width;
        legacyConfig.resolutionHeight = newResolution.height;

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
        auto& cfg = Config::get().old;
        auto& cfgNew = Config::get();

        if (!(cfgNew.display.fullscreenResolution.isPositive() && cfg.resolutionWidth > 0 && cfg.resolutionHeight > 0))
        {
            cfg.resolutionWidth = resolutions.back().width;
            cfg.resolutionHeight = resolutions.back().height;
            cfgNew.display.fullscreenResolution.width = resolutions.back().width;
            cfgNew.display.fullscreenResolution.height = resolutions.back().height;
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

    // 0x004CD422
    static void processMouseTool(int16_t x, int16_t y)
    {
        if (!Input::hasFlag(Input::Flags::toolActive))
        {
            return;
        }

        auto toolWindow = WindowManager::find(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber());
        if (toolWindow != nullptr)
        {
            toolWindow->callToolUpdate(ToolManager::getToolWidgetIndex(), x, y);
        }
        else
        {
            ToolManager::toolCancel();
        }
    }

    // 0x004C96E7
    void handleInput()
    {
        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_10))
        {
            Windows::CompanyWindow::openAndSetName();
        }

        if (Game::hasFlags(GameStateFlags::preferredOwnerName))
        {
            if (!isTitleMode() && !isEditorMode())
            {
                if (Tutorial::state() == Tutorial::State::none)
                {
                    CompanyManager::setPreferredName();
                }
            }
            Game::removeFlags(GameStateFlags::preferredOwnerName);
        }

        if (MultiPlayer::resetFlag(MultiPlayer::flags::flag_5))
        {
            GameCommands::LoadSaveQuitGameArgs args{};
            args.option1 = LoadSaveQuitGameArgs::Options::dontSave;
            args.option2 = LoadOrQuitMode::returnToTitlePrompt;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
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
            GameCommands::LoadSaveQuitGameArgs args{};
            args.option1 = GameCommands::LoadSaveQuitGameArgs::Options::save;
            args.option2 = LoadOrQuitMode::quitGamePrompt;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        if (Ui::dirtyBlocksInitialised())
        {
            WindowManager::callEvent8OnAllWindows();

            WindowManager::invalidateAllWindowsAfterInput();
            Input::updateCursorPosition();

            uint32_t x;
            int16_t y;
            Input::MouseButton state;
            while ((state = Input::nextMouseInput(x, y)) != Input::MouseButton::released)
            {
                if (isTitleMode() && Intro::isActive() && state == Input::MouseButton::leftPressed)
                {
                    if (Intro::state() == Intro::State::displayNotice)
                    {
                        Intro::state(Intro::State::end);
                        continue;
                    }
                    else
                    {
                        Intro::state(Intro::State::displayNoticeBegin);
                    }
                }
                Input::handleMouse(x, y, state);
            }

            if (Input::hasFlag(Input::Flags::rightMousePressed))
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

            Input::processMouseWheel();
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
        while ((state = Input::nextMouseInput(x, y)) != Input::MouseButton::released)
        {
            Input::handleMouse(x, y, state);
        }

        if (Input::hasFlag(Input::Flags::rightMousePressed))
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
        auto& config = Config::get();
        newScaleFactor = std::clamp(newScaleFactor, ScaleFactor::min, ScaleFactor::max);
        if (config.scaleFactor == newScaleFactor)
            return;

        config.scaleFactor = newScaleFactor;

        OpenLoco::Config::write();
        Ui::triggerResize();
        Gfx::invalidateScreen();
    }

    void adjustWindowScale(float adjust_by)
    {
        auto& config = Config::get();
        float newScaleFactor = std::clamp(config.scaleFactor + adjust_by, ScaleFactor::min, ScaleFactor::max);
        if (config.scaleFactor == newScaleFactor)
            return;

        setWindowScaling(newScaleFactor);
    }

    bool hasInputFocus()
    {
        const uint32_t windowFlags = SDL_GetWindowFlags(window);
        return (windowFlags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

}
