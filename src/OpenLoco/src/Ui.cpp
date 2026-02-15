#include "GameStateFlags.h"
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

#include <SDL3/SDL.h>

#include "Config.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Gui.h"
#include "Input.h"
#include "Intro.h"
#include "Logging.h"
#include "MultiPlayer.h"
#include "SceneManager.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/ToolManager.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Utility/String.hpp>

using namespace OpenLoco::GameCommands;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Ui
{
    struct WindowParams
    {
        int32_t x{};
        int32_t y{};
        int32_t width{};
        int32_t height{};
        int32_t flags{};
    };

#ifdef _WIN32
    static void* _hwnd;
#endif // _WIN32

    static bool _resolutionsAllowAnyAspectRatio = false;
    static std::vector<Resolution> _fsResolutions;

    static SDL_Window* _window;
    static std::map<CursorId, SDL_Cursor*> _cursors;
    static CursorId _currentCursor = CursorId::pointer;

    static void setWindowIcon();
    static Config::Resolution getDisplayResolutionByMode(Config::ScreenMode mode);

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

    // Returns the width of the game screen, which is scaled by the window scale factor.
    int32_t width()
    {
        const auto& screenInfo = Gfx::getDrawingEngine().getScreenInfo();
        return screenInfo.width;
    }

    // Returns the height of the game screen, which is scaled by the window scale factor.
    int32_t height()
    {
        const auto& screenInfo = Gfx::getDrawingEngine().getScreenInfo();
        return screenInfo.height;
    }

    bool isInitialized()
    {
        return _window != nullptr;
    }

    static SDL_PropertiesID getWindowProps(const Config::Display& cfg)
    {
        SDL_PropertiesID props = SDL_CreateProperties();
        if (props == 0)
        {
            throw Exception::RuntimeError(SDL_GetError());
        }

        int32_t x = SDL_WINDOWPOS_CENTERED_DISPLAY(cfg.index);
        int32_t y = SDL_WINDOWPOS_CENTERED_DISPLAY(cfg.index);
        int32_t width = std::max(640, cfg.windowResolution.width);
        int32_t height = std::max(480, cfg.windowResolution.height);
        uint32_t flags = SDL_WINDOW_RESIZABLE;

#if !(defined(__APPLE__) && defined(__MACH__))
        switch (cfg.mode)
        {
            case Config::ScreenMode::window:
                break;
            case Config::ScreenMode::fullscreen:
                width = cfg.fullscreenResolution.width;
                height = cfg.fullscreenResolution.height;
                flags |= SDL_WINDOW_FULLSCREEN;
                break;
            case Config::ScreenMode::fullscreenBorderless:
                // TODO: SDL separates this as exclusive fullscreen.
                // flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                break;
        }
#endif

        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "OpenLoco");
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, x);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);

        return props;
    }

    // 0x00405409
    void createWindow(const Config::Display& cfg)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            throw Exception::RuntimeError("Unable to initialise SDL2 video subsystem.");
        }

        // Create the window
        auto props = getWindowProps(cfg);
        _window = SDL_CreateWindowWithProperties(props);

        if (_window == nullptr)
        {
            SDL_DestroyProperties(props);
            throw Exception::RuntimeError("Unable to create SDL3 window.");
        }

#ifdef _WIN32
        // Grab the HWND
        _hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif

        setWindowIcon();

        auto width = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
        auto height = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);

        // Create a palette for the window
        auto& drawingEngine = Gfx::getDrawingEngine();
        drawingEngine.initialize(_window);
        drawingEngine.resize(width, height);

        // SDL2 always activated text input by default on desktop platforms, SDL3 does not.
        // TODO: Do this properly and activate/deactivate depending on textbox focus, we should also
        // set the input rectangle to avoid IME issues.
        SDL_StartTextInput(_window);

        SDL_DestroyProperties(props);
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
                auto hwnd = (HWND)_hwnd;
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
        SDL_RestoreWindow(_window);
    }

    static SDL_Cursor* loadCursor(Cursor& cursor)
    {
        return SDL_CreateCursor(cursor.data, cursor.mask, 32, 32, cursor.x, cursor.y);
    }

    // 0x00452001
    void initialiseCursors()
    {
        _cursors[CursorId::pointer] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
        _cursors[CursorId::blank] = loadCursor(Cursor::blank);
        _cursors[CursorId::upArrow] = loadCursor(Cursor::upArrow);
        _cursors[CursorId::upDownArrow] = loadCursor(Cursor::upDownArrow);
        _cursors[CursorId::handPointer] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
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
            SDL_DestroyCursor(cursor.second);
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
    Point getCursorPosScaled()
    {
        auto unscaledPos = getCursorPos();

        auto scale = Config::get().scaleFactor;

        auto x = unscaledPos.x / scale;
        auto y = unscaledPos.y / scale;
        return { static_cast<int32_t>(std::round(x)), static_cast<int32_t>(std::round(y)) };
    }

    Point getCursorPos()
    {
        float x = 0, y = 0;
        SDL_GetMouseState(&x, &y);
        return { static_cast<int32_t>(x), static_cast<int32_t>(y) };
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
        SDL_WarpMouseInWindow(_window, x, y);
    }

    void hideCursor()
    {
        SDL_HideCursor();
    }

    void showCursor()
    {
        SDL_ShowCursor();
    }

    // 0x004524C1
    void update()
    {
    }

    void windowPositionChanged([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y)
    {
        auto displayIndex = SDL_GetDisplayForWindow(_window);

        auto& cfg = Config::get().display;
        if (cfg.index != displayIndex)
        {
            cfg.index = displayIndex;
            Config::write();
        }
    }

    void windowSizeChanged(int32_t width, int32_t height)
    {
        auto& drawingEngine = Gfx::getDrawingEngine();
        drawingEngine.resize(width, height);

        Gui::resize();
        Gfx::invalidateScreen();

        // Save window size to config if NOT maximized
        auto wf = SDL_GetWindowFlags(_window);
        if (!(wf & SDL_WINDOW_MAXIMIZED) && !(wf & SDL_WINDOW_FULLSCREEN))
        {
            auto& cfg = Config::get().display;
            cfg.windowResolution = { width, height };
            Config::write();
        }
    }

    void triggerResize()
    {
        int width, height;
        SDL_GetWindowSize(_window, &width, &height);
        windowSizeChanged(width, height);
    }

    void render()
    {
        if (_window == nullptr)
        {
            return;
        }

        auto& drawingEngine = Gfx::getDrawingEngine();

        if (!Ui::isInitialized())
        {
            return;
        }

        if (!Intro::isActive())
        {
            drawingEngine.render();
        }

        drawingEngine.present();
    }

    void showMessageBox(const std::string& title, const std::string& message)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, title.c_str(), message.c_str(), _window);
    }

    static Config::Resolution getDisplayResolutionByMode(Config::ScreenMode mode)
    {
        auto& config = Config::get();
        if (mode == Config::ScreenMode::window)
        {
            if (config.display.windowResolution.isPositive())
            {
                return config.display.windowResolution;
            }
            else
            {
                return { 800, 600 };
            }
        }
        else if (mode == Config::ScreenMode::fullscreen && config.display.fullscreenResolution.isPositive())
        {
            return config.display.fullscreenResolution;
        }
        else
        {
            return getDesktopResolution();
        }
    }

    Config::Resolution getResolution()
    {
        return { Ui::width(), Ui::height() };
    }

    Config::Resolution getDesktopResolution()
    {
        int32_t displayIndex = SDL_GetDisplayForWindow(_window);

        const SDL_DisplayMode* desktopDisplayMode = SDL_GetDesktopDisplayMode(displayIndex);
        if (desktopDisplayMode == nullptr)
        {
            Logging::error("SDL_GetDesktopDisplayMode failed: {}", SDL_GetError());
            return { 800, 600 };
        }

        return { desktopDisplayMode->w, desktopDisplayMode->h };
    }

    bool setDisplayMode(Config::ScreenMode mode, Config::Resolution newResolution)
    {
        // *HACK* Set window to non fullscreen before switching resolution.
        // This fixes issues with high dpi and Windows scaling affecting the GUI size.
        SDL_SetWindowFullscreen(_window, false);

        // Set the new dimensions of the screen.
        if (mode == Config::ScreenMode::window)
        {
            auto desktopResolution = getDesktopResolution();
            auto x = (desktopResolution.width - newResolution.width) / 2;
            auto y = (desktopResolution.height - newResolution.height) / 2;
            SDL_SetWindowPosition(_window, x, y);

            SDL_SetWindowSize(_window, newResolution.width, newResolution.height);
        }
        else
        {
            SDL_SetWindowPosition(_window, 0, 0);
            SDL_SetWindowSize(_window, newResolution.width, newResolution.height);

            const auto borderless = mode == Config::ScreenMode::fullscreenBorderless;

            SDL_DisplayMode dpMode{};
            dpMode.format = SDL_GetWindowSurface(_window)->format;
            dpMode.w = newResolution.width;
            dpMode.h = newResolution.height;
            dpMode.refresh_rate = 0.0f;

            if (SDL_SetWindowFullscreenMode(_window, borderless ? nullptr : &dpMode) != 0)
            {
                Logging::error("SDL_SetWindowDisplayMode failed: {}", SDL_GetError());
                return false;
            }

            // Set the window fullscreen mode.
            if (SDL_SetWindowFullscreen(_window, true) != 0)
            {
                Logging::error("SDL_SetWindowFullscreen failed: {}", SDL_GetError());
                return false;
            }
        }

        // It appears we were successful in setting the screen mode, so let's up date the config.
        auto& config = Config::get();
        config.display.mode = mode;

        if (mode == Config::ScreenMode::window)
        {
            config.display.windowResolution = newResolution;
        }
        else if (mode == Config::ScreenMode::fullscreen)
        {
            config.display.fullscreenResolution = newResolution;
        }

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
        int32_t displayIndex = SDL_GetDisplayForWindow(_window);

        // Get desktop aspect ratio
        const SDL_DisplayMode* desktopMode = SDL_GetDesktopDisplayMode(displayIndex);
        float desktopAspectRatio = (float)desktopMode->w / desktopMode->h;

        // Get resolutions
        int32_t numDisplayModes{};
        SDL_DisplayMode** displayModes = SDL_GetFullscreenDisplayModes(displayIndex, &numDisplayModes);
        if (!displayModes || numDisplayModes <= 0)
        {
            Logging::error("SDL_GetFullscreenDisplayModes failed: {}", SDL_GetError());
            return;
        }

        auto resolutions = std::vector<Resolution>();
        for (int32_t i = 0; i < numDisplayModes; i++)
        {
            const auto* mode = displayModes[i];
            if (mode->w > 0 && mode->h > 0)
            {
                float aspectRatio = (float)mode->w / mode->h;
                if (_resolutionsAllowAnyAspectRatio || std::fabs(desktopAspectRatio - aspectRatio) < 0.0001f)
                {
                    resolutions.push_back({ mode->w, mode->h });
                }
            }
        }

        SDL_free(displayModes);

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

        // Update configured fullscreen resolution if not set
        auto& cfg = Config::get().display.fullscreenResolution;
        if (!cfg.isPositive())
        {
            cfg.width = resolutions.back().width;
            cfg.height = resolutions.back().height;
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
    void toggleFullscreenDesktop()
    {
        auto flags = SDL_GetWindowFlags(_window);
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
            // TODO: Use widget ids properly for tools.
            toolWindow->callToolUpdate(ToolManager::getToolWidgetIndex(), WidgetId::none, x, y);
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
            if (!SceneManager::isTitleMode() && !SceneManager::isEditorMode())
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
            args.loadQuitMode = LoadOrQuitMode::returnToTitlePrompt;
            args.saveMode = LoadSaveQuitGameArgs::SaveMode::dontSave;
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
            args.loadQuitMode = LoadOrQuitMode::quitGamePrompt;
            args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        if (Ui::isInitialized())
        {
            WindowManager::callEvent8OnAllWindows();

            WindowManager::invalidateAllWindowsAfterInput();
            Input::updateCursorPosition();

            int32_t x;
            int32_t y;
            Input::MouseButton state;
            while ((state = Input::nextMouseInput(x, y)) != Input::MouseButton::released)
            {
                if (SceneManager::isTitleMode() && Intro::isActive() && state == Input::MouseButton::leftPressed)
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
            else if (x >= 0)
            {
                x = std::clamp(x, 0, Ui::width() - 1);
                y = std::clamp(y, 0, Ui::height() - 1);

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

        int32_t x;
        int32_t y;
        Input::MouseButton state;
        while ((state = Input::nextMouseInput(x, y)) != Input::MouseButton::released)
        {
            Input::handleMouse(x, y, state);
        }

        if (Input::hasFlag(Input::Flags::rightMousePressed))
        {
            Input::handleMouse(x, y, state);
        }
        else if (x >= 0)
        {
            x = std::clamp(x, 0, Ui::width() - 1);
            y = std::clamp(y, 0, Ui::height() - 1);

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
        {
            return;
        }

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
        {
            return;
        }

        setWindowScaling(newScaleFactor);
    }

    bool hasInputFocus()
    {
        const uint32_t windowFlags = SDL_GetWindowFlags(_window);
        return (windowFlags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

}
