#include "WindowManager.h"
#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Console.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
#include "../MultiPlayer.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Tutorial.h"
#include "../Ui.h"
#include "../Vehicles/Vehicle.h"
#include "../ViewportManager.h"
#include "../Widget.h"
#include "ScrollView.h"
#include <algorithm>
#include <cinttypes>
#include <memory>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::WindowManager
{
    namespace FindFlag
    {
        constexpr uint16_t byType = 1 << 7;
    }

    constexpr size_t max_windows = 12;

    static loco_global<uint16_t, 0x0050C19C> _timeSinceLastTick;
    static loco_global<uint16_t, 0x0052334E> _thousandthTickCounter;
    static loco_global<WindowType, 0x00523364> _callingWindowType;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x00508F10> __508F10;
    static loco_global<Gfx::Context, 0x0050B884> _screenContext;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIdx;
    static loco_global<uint8_t, 0x005233B6> _currentModalType;
    static loco_global<uint32_t, 0x00523508> _523508;
    static loco_global<int32_t, 0x00525330> _cursorWheel;
    static loco_global<uint32_t, 0x009DA3D4> _9DA3D4;
    static loco_global<int32_t, 0x00E3F0B8> _gCurrentRotation;
    static loco_global<Window[max_windows], 0x011370AC> _windows;
    static loco_global<Window*, 0x0113D754> _windowsEnd;

    static void viewportRedrawAfterShift(Window* window, Viewport* viewport, int16_t x, int16_t y);

    void init()
    {
        _windowsEnd = &_windows[0];
        _523508 = 0;
    }

    void registerHooks()
    {
        registerHook(
            0x0043454F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Windows::CompanyWindow::open(CompanyId(regs.ax));
                regs = backup;

                return 0;
            });

        registerHook(
            0x004345EE,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Windows::CompanyWindow::openFinances(CompanyId(regs.ax));
                regs = backup;

                return 0;
            });

        registerHook(
            0x00434731,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Windows::CompanyWindow::openChallenge(CompanyId(regs.ax));
                regs = backup;

                return 0;
            });

        registerHook(
            0x0043CB9F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Windows::TitleMenu::editorInit();

                return 0;
            });

        registerHook(
            0x0043EE58,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Windows::ScenarioOptions::open();
                regs = backup;

                return 0;
            });

        registerHook(
            0x004B6033,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto* w = Windows::Vehicle::Main::open(reinterpret_cast<Vehicles::VehicleBase*>(regs.edx));
                regs = backup;
                regs.esi = X86Pointer(w);
                return 0;
            });

        registerHook(
            0x0045EFDB,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (Ui::Window*)regs.esi;
                window->viewportZoomIn(false);
                regs = backup;
                return 0;
            });

        registerHook(
            0x0045F015,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (Ui::Window*)regs.esi;
                window->viewportZoomOut(false);
                regs = backup;
                return 0;
            });

        registerHook(
            0x0045F18B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                callViewportRotateEventOnAllWindows();
                regs = backup;

                return 0;
            });

        registerHook(
            0x0045FCE6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Point mouse = { regs.ax, regs.bx };
                auto pos = Ui::screenGetMapXyWithZ(mouse, regs.bp);
                regs = backup;
                if (pos)
                {
                    regs.ax = (*pos).x;
                    regs.bx = (*pos).y;
                }
                else
                {
                    regs.ax = -32768;
                }
                return 0;
            });

        registerHook(
            0x004610F2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Map::TileManager::mapInvalidateSelectionRect();
                regs = backup;

                return 0;
            });

        registerHook(
            0x00456D2D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Windows::Industry::open(IndustryId(regs.dl));
                regs = backup;

                return 0;
            });

        registerHook(
            0x00495685,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                const char* buffer = (const char*)regs.esi;
                uint16_t width = Gfx::getStringWidth(buffer);
                regs = backup;
                regs.cx = width;

                return 0;
            });

        registerHook(
            0x00499B7E,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = Windows::Town::open(regs.dx);
                regs = backup;
                regs.esi = X86Pointer(window);

                return 0;
            });

        registerHook(
            0x0048F210,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = Windows::Station::open(StationId(regs.dx));
                regs = backup;
                regs.esi = X86Pointer(window);

                return 0;
            });

        registerHook(
            0x004577FF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = Windows::IndustryList::open();
                regs = backup;
                regs.esi = X86Pointer(window);

                return 0;
            });

        registerHook(
            0x00428F8B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Ui::Windows::NewsWindow::open(MessageId(regs.ax));
                regs = backup;

                return 0;
            });

        registerHook(
            0x004B93A5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4B93A5(regs.bx);
                regs = backup;

                return 0;
            });

        registerHook(
            0x004C5C69,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Gfx::setDirtyBlocks(regs.ax, regs.bx, regs.dx, regs.bp);
                regs = backup;

                return 0;
            });

        registerHook(
            0x004C9984,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidateAllWindowsAfterInput();
                regs = backup;

                return 0;
            });

        registerHook(
            0x004C9A95,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = findAt(regs.ax, regs.bx);
                regs = backup;
                regs.esi = X86Pointer(window);

                return 0;
            });

        registerHook(
            0x004C9AFA,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = findAtAlt(regs.ax, regs.bx);
                regs = backup;
                regs.esi = X86Pointer(window);

                return 0;
            });

        registerHook(
            0x004C9B56,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Ui::Window* w;
                if (regs.cx & FindFlag::byType)
                {
                    w = find((WindowType)(regs.cx & ~FindFlag::byType));
                }
                else
                {
                    w = find((WindowType)regs.cx, regs.dx);
                }

                regs.esi = X86Pointer(w);
                if (w == nullptr)
                {
                    return X86_FLAG_ZERO;
                }

                return 0;
            });

        registerHook(
            0x004CA4BD,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (Ui::Window*)regs.esi;
                if (window != nullptr)
                {
                    window->invalidate();
                }
                regs = backup;
                return 0;
            });

        registerHook(
            0x004CB966,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                if (regs.al < 0)
                {
                    invalidateWidget((WindowType)(regs.al & 0x7F), regs.bx, regs.ah);
                }
                else if ((regs.al & 1 << 6) != 0)
                {
                    invalidate((WindowType)(regs.al & 0xBF));
                }
                else
                {
                    invalidate((WindowType)regs.al, regs.bx);
                }
                regs = backup;

                return 0;
            });

        registerHook(
            0x004CC692,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                if ((regs.cx & FindFlag::byType) != 0)
                {
                    close((WindowType)(regs.cx & ~FindFlag::byType));
                }
                else
                {
                    close((WindowType)regs.cx, regs.dx);
                }
                regs = backup;

                return 0;
            });

        registerHook(
            0x004CC6EA,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (Ui::Window*)regs.esi;
                close(window);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004CD296,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                relocateWindows();
                regs = backup;

                return 0;
            });

        registerHook(
            0x004CD3D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                dispatchUpdateAll();
                return 0;
            });

        registerHook(
            0x004CE3D6,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Input::toolCancel();
                regs = backup;

                return 0;
            });

        registerHook(
            0x004CE438,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto w = getMainWindow();

                regs.esi = X86Pointer(w);
                if (w == nullptr)
                {
                    return X86_FLAG_CARRY;
                }

                return 0;
            });

        registerHook(
            0x004CEE0B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4CEE0B((Ui::Window*)regs.esi);
                regs = backup;

                return 0;
            });

        registerHook(
            0x004C9F5D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto w = createWindow((WindowType)regs.cl, Ui::Point(regs.ax, regs.eax >> 16), Ui::Size(regs.bx, regs.ebx >> 16), regs.ecx >> 8, (WindowEventList*)regs.edx);
                regs = backup;

                regs.esi = X86Pointer(w);
                return 0;
            });

        registerHook(
            0x004C9C68,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto w = createWindow((WindowType)regs.cl, Ui::Size(regs.bx, (((uint32_t)regs.ebx) >> 16)), regs.ecx >> 8, (WindowEventList*)regs.edx);
                regs = backup;

                regs.esi = X86Pointer(w);
                return 0;
            });

        registerHook(
            0x004CF456,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                closeAllFloatingWindows();
                regs = backup;

                return 0;
            });

        registerHook(
            0x004CD3A9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto w = bringToFront((WindowType)regs.cx, regs.dx);
                regs = backup;

                regs.esi = X86Pointer(w);
                if (w == nullptr)
                {
                    return X86_FLAG_ZERO;
                }

                return 0;
            });
    }

    Window* get(size_t index)
    {
        return &_windows[index];
    }

    size_t indexOf(Window* pWindow)
    {
        int i = 0;

        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w == pWindow)
                return i;

            i++;
        }

        return 0;
    }

    size_t count()
    {
        return ((uintptr_t)*_windowsEnd - (uintptr_t)_windows.get()) / sizeof(Window);
    }

    WindowType getCurrentModalType()
    {
        return (WindowType)*_currentModalType;
    }

    void setCurrentModalType(WindowType type)
    {
        _currentModalType = (uint8_t)type;
    }

    void updateViewports()
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            w->viewportsUpdatePosition();
        }
    }

    // 0x004C6118
    void update()
    {
        _tooltipNotShownTicks = _tooltipNotShownTicks + _timeSinceLastTick;

        // 1000 tick update
        _thousandthTickCounter = _thousandthTickCounter + _timeSinceLastTick;
        if (_thousandthTickCounter >= 1000)
        {
            _thousandthTickCounter = 0;
            for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
            {
                w->callOnPeriodicUpdate();
            }
        }

        // Border flash invalidation
        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if ((w->flags & WindowFlags::whiteBorderMask) != 0)
            {
                // TODO: Replace with countdown
                w->flags -= WindowFlags::whiteBorderOne;
                if ((w->flags & WindowFlags::whiteBorderMask) == 0)
                {
                    w->invalidate();
                }
            }
        }

        allWheelInput();
    }

    // 0x00439BA5
    void updateDaily()
    {
        call(0x00439BA5);
    }

    // 0x004CE438
    Window* getMainWindow()
    {
        return find(WindowType::main);
    }

    Viewport* getMainViewport()
    {
        auto mainWindow = getMainWindow();
        if (mainWindow != nullptr)
        {
            return mainWindow->viewports[0];
        }
        return nullptr;
    }

    // 0x004C9B56
    Window* find(WindowType type)
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type == type)
            {
                return w;
            }
        }

        return nullptr;
    }

    // 0x004C9B56
    Window* find(WindowType type, WindowNumber_t number)
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type == type && w->number == number)
            {
                return w;
            }
        }

        return nullptr;
    }

    // 0x004C9A95
    Window* findAt(int16_t x, int16_t y)
    {
        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (x < w->x)
                continue;

            if (x >= (w->x + w->width))
                continue;

            if (y < w->y)
                continue;
            if (y >= (w->y + w->height))
                continue;

            if ((w->flags & WindowFlags::flag_7) != 0)
                continue;

            if ((w->flags & WindowFlags::noBackground) != 0)
            {
                auto index = w->findWidgetAt(x, y);
                if (index == -1)
                {
                    continue;
                }
            }

            if (w->callOnResize() == nullptr)
            {
                return findAt(x, y);
            }

            return w;
        }

        return nullptr;
    }

    Window* findAt(Ui::Point point)
    {
        return findAt(point.x, point.y);
    }

    // 0x004C9AFA
    Window* findAtAlt(int16_t x, int16_t y)
    {
        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (x < w->x)
                continue;

            if (x >= (w->x + w->width))
                continue;

            if (y < w->y)
                continue;
            if (y >= (w->y + w->height))
                continue;

            if ((w->flags & WindowFlags::noBackground) != 0)
            {
                auto index = w->findWidgetAt(x, y);
                if (index == -1)
                {
                    continue;
                }
            }

            if (w->callOnResize() == nullptr)
            {
                return findAtAlt(x, y);
            }

            return w;
        }

        return nullptr;
    }

    // 0x004CB966
    void invalidate(WindowType type)
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != type)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidate(WindowType type, WindowNumber_t number)
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidateWidget(WindowType type, WindowNumber_t number, uint8_t widget_index)
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            auto widget = w->widgets[widget_index];

            if (widget.left != -2)
            {
                Gfx::setDirtyBlocks(
                    w->x + widget.left,
                    w->y + widget.top,
                    w->x + widget.right + 1,
                    w->y + widget.bottom + 1);
            }
        }
    }

    // 0x004C9984
    void invalidateAllWindowsAfterInput()
    {
        if (isPaused())
        {
            _523508++;
        }

        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            w->updateScrollWidgets();
            w->invalidatePressedImageButtons();
            w->callOnResize();
        }
    }

    // 0x004CC692
    void close(WindowType type)
    {
        bool repeat = true;
        while (repeat)
        {
            repeat = false;
            for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
            {
                if (w->type != type)
                    continue;

                close(w);
                repeat = true;
                break;
            }
        }
    }

    // 0x004CC692
    void close(WindowType type, WindowNumber_t id)
    {
        auto window = find(type, id);
        if (window != nullptr)
        {
            close(window);
        }
    }

    // 0x004CC750
    // TODO: hook
    Window* bringToFront(Window* w)
    {
        if (w->flags & (WindowFlags::stickToBack | WindowFlags::stickToFront))
        {
            return w;
        }

        Window* frontMostWnd = nullptr;
        for (auto i = count(); i != 0; --i)
        {
            if (!(_windows[i - 1].flags & WindowFlags::stickToBack))
            {
                frontMostWnd = &_windows[i - 1];
                break;
            }
        }
        if (frontMostWnd != nullptr && frontMostWnd != &_windows[0] && frontMostWnd != w)
        {
            std::swap(*frontMostWnd, *w);
            w = frontMostWnd;
            w->invalidate();
        }

        const auto right = w->x + w->width;
        // If window is almost offscreen to the left
        if (right < 20)
        {
            const auto shiftRight = 20 - w->x;
            w->x = 20;
            for (auto* vp : w->viewports)
            {
                if (vp != nullptr)
                {
                    vp->x += shiftRight;
                }
            }
            w->invalidate();
        }

        return w;
    }

    // 0x004CD3A9
    Window* bringToFront(WindowType type, uint16_t id)
    {
        auto window = find(type, id);
        if (window == nullptr)
            return nullptr;

        window->flags |= WindowFlags::whiteBorderMask;
        window->invalidate();

        return bringToFront(window);
    }

    /**
     * 0x004C9BEA
     *
     * @param x @<dx>
     * @param y @<ax>
     * @param width @<bx>
     * @param height @<cx>
     */
    static bool windowFitsWithinSpace(Ui::Point position, Ui::Size size)
    {
        if (position.x < 0)
            return false;

        if (position.y < 28)
            return false;

        if (position.x + size.width > Ui::width())
            return false;

        if (position.y + size.height > Ui::width())
            return false;

        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if ((w->flags & WindowFlags::stickToBack) != 0)
                continue;
            if (position.x + size.width <= w->x)
                continue;
            if (position.x > w->x + w->width)
                continue;
            if (position.y + size.height <= w->y)
                continue;
            if (position.y >= w->y + w->height)
                continue;

            return false;
        }

        return true;
    }

    // 0x004C9F27
    static Window* createWindowOnScreen(
        WindowType type,
        Ui::Point origin,
        Ui::Size size,
        uint32_t flags,
        WindowEventList* events)
    {
        origin.x = std::clamp<decltype(origin.x)>(origin.x, 0, std::max(0, Ui::width() - size.width));
        origin.y = std::clamp<decltype(origin.y)>(origin.y, 28, std::max(28, Ui::height() - size.height));

        return createWindow(type, origin, size, flags, events);
    }

    // 0x004C9BA2
    static bool windowFitsOnScreen(Ui::Point origin, Ui::Size size)
    {
        if (origin.x < -(size.width / 4))
            return false;
        if (origin.x > Ui::width() - (size.width / 2))
            return false;

        if (origin.y < 28)
            return false;
        if (origin.y > Ui::height() - (size.height / 4))
            return false;

        return windowFitsWithinSpace(origin, size);
    }

    /**
     * 0x004C9C68
     *
     * @param type @<cl>
     * @param size.width @<bx>
     * @param size.height @<ebx>
     * @param flags @<ecx << 8>
     * @param events @<edx>
     * @return
     */
    Window* createWindow(
        WindowType type,
        Ui::Size size,
        uint32_t flags,
        WindowEventList* events)
    {
        Ui::Point position{};

        position.x = 0;  // dx
        position.y = 30; // ax
        if (windowFitsWithinSpace(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        position.x = Ui::width() - size.width;
        position.y = 30;
        if (windowFitsWithinSpace(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        position.x = 0;
        position.y = Ui::height() - size.height - 29;
        if (windowFitsWithinSpace(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        position.x = Ui::width() - size.width;
        position.y = Ui::height() - size.height - 29;
        if (windowFitsWithinSpace(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->flags & WindowFlags::stickToBack)
                continue;

            position.x = w->x + w->width + 2;
            position.y = w->y;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x - size.width - 2;
            position.y = w->y;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y + w->height + 2;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y - size.height - 2;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x + w->width + 2;
            position.y = w->y + w->height - size.height;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x - size.width - 2;
            position.y = w->y + w->height - size.height;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x + w->width - size.width;
            position.y = w->y - size.height - 2;
            if (windowFitsWithinSpace(position, size))
                return createWindowOnScreen(type, position, size, flags, events);
        }

        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->flags & WindowFlags::stickToBack)
                continue;

            position.x = w->x + w->width + 2;
            position.y = w->y;
            if (windowFitsOnScreen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x - size.width - 2;
            position.y = w->y;
            if (windowFitsOnScreen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y + w->height + 2;
            if (windowFitsOnScreen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y - size.height - 2;
            if (windowFitsOnScreen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);
        }

        position.x = 0;
        position.y = 30;

        bool loop;
        do
        {
            loop = false;
            for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
            {
                if (w->x == position.x && w->y == position.y)
                {
                    position.x += 5;
                    position.y += 5;
                    // restart loop
                    loop = true;
                    break;
                }
            }
        } while (loop);

        return createWindowOnScreen(type, position, size, flags, events);
    }

    /**
     * 0x004C9F5D
     *
     * @param type @<cl>
     * @param origin @<eax>
     * @param size @<ebx>
     * @param flags @<ecx << 8>
     * @param events @<edx>
     * @return
     */
    Window* createWindow(
        WindowType type,
        Ui::Point origin,
        Ui::Size size,
        uint32_t flags,
        WindowEventList* events)
    {
        if (count() == max_windows)
        {
            for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
            {
                if ((w->flags & WindowFlags::stickToBack) != 0)
                    continue;

                if ((w->flags & WindowFlags::stickToFront) != 0)
                    continue;

                if ((w->flags & WindowFlags::noAutoClose) != 0)
                    continue;

                close(w);
                return createWindow(type, origin, size, flags, events);
            }
        }

        bool stickToBack = (flags & WindowFlags::stickToBack) != 0;
        bool stickToFront = (flags & WindowFlags::stickToFront) != 0;
        bool hasFlag12 = (flags & WindowFlags::flag_12) != 0;
        bool shouldOpenQuietly = (flags & WindowFlags::openQuietly) != 0;

        // Find right position to insert new window
        size_t dstIndex = 0;
        if (stickToBack)
        {
            for (size_t i = 0; i < count(); i++)
            {
                if ((_windows[i].flags & WindowFlags::stickToBack) != 0)
                {
                    dstIndex = i;
                }
            }
        }
        else if (stickToFront)
        {
            dstIndex = count();
        }
        else
        {
            for (int i = (int)count(); i > 0; i--)
            {
                if ((_windows[i - 1].flags & WindowFlags::stickToFront) == 0)
                {
                    dstIndex = i;
                    break;
                }
            }
        }

        auto window = Ui::Window(origin, size);
        window.type = type;
        window.flags = flags;
        if (hasFlag12 || (!stickToBack && !stickToFront && !shouldOpenQuietly))
        {
            window.flags |= WindowFlags::whiteBorderMask;
            Audio::playSound(Audio::SoundId::openWindow, origin.x + size.width / 2);
        }

        window.eventHandlers = events;

        size_t length = _windowsEnd - (_windows + dstIndex);
        memmove(_windows + dstIndex + 1, _windows + dstIndex, length * sizeof(Ui::Window));
        _windowsEnd = _windowsEnd + 1;
        _windows[dstIndex] = window;

        _windows[dstIndex].invalidate();

        return &_windows[dstIndex];
    }

    Window* createWindowCentred(WindowType type, Ui::Size size, uint32_t flags, WindowEventList* events)
    {
        auto x = (Ui::width() / 2) - (size.width / 2);
        auto y = std::max(28, (Ui::height() / 2) - (size.height / 2));
        return createWindow(type, Ui::Point(x, y), size, flags, events);
    }

    // 0x004C5FC8
    void drawSingle(Gfx::Context* _context, Window* w, int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        // Copy context so we can crop it
        auto context = *_context;

        // Clamp left to 0
        int32_t overflow = left - context.x;
        if (overflow > 0)
        {
            context.x += overflow;
            context.width -= overflow;
            if (context.width <= 0)
                return;
            context.pitch += overflow;
            context.bits += overflow;
        }

        // Clamp width to right
        overflow = context.x + context.width - right;
        if (overflow > 0)
        {
            context.width -= overflow;
            if (context.width <= 0)
                return;
            context.pitch += overflow;
        }

        // Clamp top to 0
        overflow = top - context.y;
        if (overflow > 0)
        {
            context.y += overflow;
            context.height -= overflow;
            if (context.height <= 0)
                return;
            context.bits += (context.width + context.pitch) * overflow;
        }

        // Clamp height to bottom
        overflow = context.y + context.height - bottom;
        if (overflow > 0)
        {
            context.height -= overflow;
            if (context.height <= 0)
                return;
        }

        if (isProgressBarActive() && w->type != WindowType::progressBar)
        {
            return;
        }

        // Company colour
        if (w->owner != CompanyId::null)
        {
            w->setColour(WindowColour::primary, static_cast<Colour>(CompanyManager::getCompanyColour(w->owner)));
        }

        addr<0x1136F9C, int16_t>() = w->x;
        addr<0x1136F9E, int16_t>() = w->y;

        loco_global<AdvancedColour[4], 0x1136594> windowColours;
        // Text colouring
        windowColours[0] = w->getColour(WindowColour::primary).opaque();
        windowColours[1] = w->getColour(WindowColour::secondary).opaque();
        windowColours[2] = w->getColour(WindowColour::tertiary).opaque();
        windowColours[3] = w->getColour(WindowColour::quaternary).opaque();

        w->callPrepareDraw();
        w->callDraw(&context);
    }

    // 0x004CD3D0
    void dispatchUpdateAll()
    {
        _523508++;
        CompanyManager::setUpdatingCompanyId(CompanyManager::getControllingId());

        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            w->callUpdate();
        }

        Ui::Windows::TextInput::sub_4CE6FF();
        Ui::Windows::MapToolTip::open();
    }

    // 0x004CC6EA
    void close(Window* window)
    {
        if (window == nullptr)
        {
            return;
        }

        // Make a copy of the window class and number in case
        // the window order is changed by the close event.
        auto type = window->type;
        uint16_t number = window->number;

        window->callClose();

        window = find(type, number);
        if (window == nullptr)
            return;

        window->viewportRemove(0);
        window->viewportRemove(1);

        window->invalidate();

        // Remove window from list and reshift all windows
        _windowsEnd--;
        int windowCount = *_windowsEnd - window;
        if (windowCount > 0)
        {
            memmove(window, window + 1, windowCount * sizeof(Ui::Window));
        }

        ViewportManager::collectGarbage();
    }

    void callEvent8OnAllWindows()
    {
        for (Window* window = _windowsEnd - 1; window >= _windows; window--)
        {
            window->call_8();
        }
    }

    void callEvent9OnAllWindows()
    {
        for (Window* window = _windowsEnd - 1; window >= _windows; window--)
        {
            window->call_9();
        }
    }

    // 0x0045F18B
    void callViewportRotateEventOnAllWindows()
    {
        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            w->callViewportRotate();
        }
    }

    // 0x004CD296
    void relocateWindows()
    {
        int16_t newLocation = 8;
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            // Work out if the window requires moving
            bool extendsX = (w->x + 10) >= Ui::width();
            bool extendsY = (w->y + 10) >= Ui::height();
            if ((w->flags & WindowFlags::stickToBack) != 0 || (w->flags & WindowFlags::stickToFront) != 0)
            {
                // toolbars are 27px high
                extendsY = (w->y + 10 - 27) >= Ui::height();
            }

            if (extendsX || extendsY)
            {
                // Calculate the new locations
                int16_t oldX = w->x;
                int16_t oldY = w->y;
                w->x = newLocation;
                w->y = newLocation + 28;

                // Move the next new location so windows are not directly on top
                newLocation += 8;

                // Adjust the viewports if required.
                if (w->viewports[0] != nullptr)
                {
                    w->viewports[0]->x -= oldX - w->x;
                    w->viewports[0]->y -= oldY - w->y;
                }

                if (w->viewports[1] != nullptr)
                {
                    w->viewports[1]->x -= oldX - w->x;
                    w->viewports[1]->y -= oldY - w->y;
                }
            }
        }
    }

    // 0x004CEE0B
    void sub_4CEE0B(Window* self)
    {
        int left = self->x;
        int right = self->x + self->width;
        int top = self->y;
        int bottom = self->y + self->height;

        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w == self)
                continue;

            if (w->flags & WindowFlags::stickToBack)
                continue;

            if (w->flags & WindowFlags::stickToFront)
                continue;

            if (w->x >= right)
                continue;

            if (w->x + w->width <= left)
                continue;

            if (w->y >= bottom)
                continue;

            if (w->y + w->height <= top)
                continue;

            w->invalidate();

            if (bottom < Ui::height() - 80)
            {
                int dY = bottom + 3 - w->y;
                w->y += dY;
                w->invalidate();

                if (w->viewports[0] != nullptr)
                {
                    w->viewports[0]->y += dY;
                }

                if (w->viewports[1] != nullptr)
                {
                    w->viewports[1]->y += dY;
                }
            }
        }
    }

    // 0x004B93A5
    void sub_4B93A5(WindowNumber_t number)
    {
        for (Ui::Window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != WindowType::vehicle)
                continue;

            if (w->number != number)
                continue;

            if (w->currentTab != 4)
                continue;

            w->invalidate();
        }
    }

    // 0x004A0AB0
    void closeConstructionWindows()
    {
        close(WindowType::construction);
        close(WindowType::companyFaceSelection);
        Input::toolCancel();
        addr<0x00522096, uint8_t>() = 0;
    }

    // 0x004BF089
    void closeTopmost()
    {
        close(WindowType::dropdown, 0);

        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (w->flags & WindowFlags::stickToBack)
                continue;

            if (w->flags & WindowFlags::stickToFront)
                continue;

            close(w);
            break;
        }
    }

    static void windowScrollWheelInput(Ui::Window* window, WidgetIndex_t widgetIndex, int wheel)
    {
        int scrollIndex = window->getScrollDataIndex(widgetIndex);
        ScrollArea* scroll = &window->scrollAreas[scrollIndex];
        Ui::Widget* widget = &window->widgets[widgetIndex];

        if (window->scrollAreas[scrollIndex].flags & ScrollView::ScrollFlags::vscrollbarVisible)
        {
            int size = widget->bottom - widget->top - 1;
            if (scroll->flags & ScrollView::ScrollFlags::hscrollbarVisible)
                size -= 11;
            size = std::max(0, scroll->contentHeight - size);
            scroll->contentOffsetY = std::clamp(scroll->contentOffsetY + wheel, 0, size);
        }
        else if (window->scrollAreas[scrollIndex].flags & ScrollView::ScrollFlags::hscrollbarVisible)
        {
            int size = widget->right - widget->left - 1;
            if (scroll->flags & ScrollView::ScrollFlags::vscrollbarVisible)
                size -= 11;
            size = std::max(0, scroll->contentWidth - size);
            scroll->contentOffsetX = std::clamp(scroll->contentOffsetX + wheel, 0, size);
        }

        Ui::ScrollView::updateThumbs(window, widgetIndex);
        invalidateWidget(window->type, window->number, widgetIndex);
    }

    // 0x004C628E
    static bool windowWheelInput(Window* window, int wheel)
    {
        int widgetIndex = -1;
        int scrollIndex = -1;
        for (Widget* widget = window->widgets; widget->type != WidgetType::end; widget++)
        {
            widgetIndex++;

            if (widget->type != WidgetType::scrollview)
                continue;

            scrollIndex++;
            constexpr uint16_t scrollbarFlags = ScrollView::ScrollFlags::hscrollbarVisible | ScrollView::ScrollFlags::vscrollbarVisible;
            if (window->scrollAreas[scrollIndex].flags & scrollbarFlags)
            {
                windowScrollWheelInput(window, widgetIndex, wheel);
                return true;
            }
        }

        return false;
    }

    // 0x004C6202
    void allWheelInput()
    {
        int wheel = 0;

        while (true)
        {
            _cursorWheel -= 120;

            if (_cursorWheel < 0)
            {
                _cursorWheel += 120;
                break;
            }

            wheel -= 17;
        }

        while (true)
        {
            _cursorWheel += 120;

            if (_cursorWheel > 0)
            {
                _cursorWheel -= 120;
                break;
            }

            wheel += 17;
        }

        if (Tutorial::state() != Tutorial::State::none)
            return;

        if (Input::hasFlag(Input::Flags::flag5))
        {
            if (OpenLoco::isTitleMode())
                return;

            auto main = WindowManager::getMainWindow();
            if (main != nullptr && wheel != 0)
            {
                if (wheel > 0)
                {
                    main->viewportRotateRight();
                }
                else if (wheel < 0)
                {
                    main->viewportRotateLeft();
                }
                TownManager::updateLabels();
                StationManager::updateLabels();
                Windows::MapWindow::centerOnViewPoint();
            }

            return;
        }

        const Ui::Point cursorPosition = Input::getMouseLocation();
        auto window = findAt(cursorPosition);

        if (window != nullptr)
        {
            if (window->type == WindowType::main)
            {
                if (OpenLoco::isTitleMode())
                    return;

                if (wheel > 0)
                {
                    window->viewportZoomOut(true);
                }
                else if (wheel < 0)
                {
                    window->viewportZoomIn(true);
                }
                TownManager::updateLabels();
                StationManager::updateLabels();

                return;
            }
            else
            {
                auto widgetIndex = window->findWidgetAt(cursorPosition.x, cursorPosition.y);
                if (widgetIndex != -1)
                {
                    if (window->widgets[widgetIndex].type == WidgetType::scrollview)
                    {
                        auto scrollIndex = window->getScrollDataIndex(widgetIndex);
                        constexpr uint16_t scrollbarFlags = ScrollView::ScrollFlags::hscrollbarVisible | ScrollView::ScrollFlags::vscrollbarVisible;
                        if (window->scrollAreas[scrollIndex].flags & scrollbarFlags)
                        {
                            windowScrollWheelInput(window, widgetIndex, wheel);
                            return;
                        }
                    }

                    if (windowWheelInput(window, wheel))
                    {
                        return;
                    }
                }
            }
        }

        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (windowWheelInput(w, wheel))
            {
                return;
            }
        }
    }

    bool isInFront(Ui::Window* window)
    {
        for (Ui::Window* w = window + 1; w != _windowsEnd; w++)
        {
            if ((w->flags & WindowFlags::stickToFront) != 0)
                continue;

            return false;
        }

        return true;
    }

    bool isInFrontAlt(Ui::Window* window)
    {
        for (Ui::Window* w = window + 1; w != _windowsEnd; w++)
        {
            if ((w->flags & WindowFlags::stickToFront) != 0)
                continue;

            if (w->type == WindowType::buildVehicle)
                continue;

            return false;
        }

        return true;
    }

    // 0x0046960C
    Ui::Window* findWindowShowing(const viewport_pos& position)
    {
        for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (w->viewports[0] == nullptr)
                continue;

            auto viewport = w->viewports[0];
            if (viewport->zoom != 0)
                continue;

            if (!viewport->contains(position))
                continue;

            return w;
        }

        return nullptr;
    }

    /**
     * 0x004C6A40
     *
     * @param window @<edi>
     * @param viewport @<esi>
     */
    void viewportShiftPixels(Ui::Window* window, Ui::Viewport* viewport, int16_t dX, int16_t dY)
    {
        for (auto w = window; w < _windowsEnd; w++)
        {
            if ((w->flags & WindowFlags::transparent) == 0)
                continue;

            if (viewport == w->viewports[0])
                continue;

            if (viewport == w->viewports[1])
                continue;

            if (viewport->x + viewport->width <= w->x)
                continue;

            if (w->x + w->width <= viewport->x)
                continue;

            if (viewport->y + viewport->height <= w->y)
                continue;

            if (w->y + w->height <= viewport->y)
                continue;

            int16_t left, top, right, bottom, cx;

            left = w->x;
            top = w->y;
            right = w->x + w->width;
            bottom = w->y + w->height;

            // TODO: replace these with min/max
            cx = viewport->x;
            if (left < cx)
                left = cx;

            cx = viewport->x + viewport->width;
            if (right > cx)
                right = cx;

            cx = viewport->y;
            if (top < cx)
                top = cx;

            cx = viewport->y + viewport->height;
            if (bottom > cx)
                bottom = cx;

            if (left < right && top < bottom)
            {
                Gfx::redrawScreenRect(left, top, right, bottom);
            }
        }

        viewportRedrawAfterShift(window, viewport, dX, dY);
    }

    // 0x00451DCB
    static void copyRect(int16_t x, int16_t y, int16_t width, int16_t height, int16_t dx, int16_t dy)
    {
        if (dx == 0 && dy == 0)
            return;

        auto _width = Ui::width();
        auto _height = Ui::height();
        Gfx::Context& _bitsContext = _screenContext;

        // Adjust for move off screen
        // NOTE: when zooming, there can be x, y, dx, dy combinations that go off the
        // screen; hence the checks. This code should ultimately not be called when
        // zooming because this function is specific to updating the screen on move
        int32_t lmargin = std::min(x - dx, 0);
        int32_t rmargin = std::min((int32_t)_width - (x - dx + width), 0);
        int32_t tmargin = std::min(y - dy, 0);
        int32_t bmargin = std::min((int32_t)_height - (y - dy + height), 0);
        x -= lmargin;
        y -= tmargin;
        width += lmargin + rmargin;
        height += tmargin + bmargin;

        int32_t stride = _bitsContext.width + _bitsContext.pitch;
        uint8_t* to = _bitsContext.bits + y * stride + x;
        uint8_t* from = _bitsContext.bits + (y - dy) * stride + x - dx;

        if (dy > 0)
        {
            // If positive dy, reverse directions
            to += (height - 1) * stride;
            from += (height - 1) * stride;
            stride = -stride;
        }

        // Move bytes
        for (int32_t i = 0; i < height; i++)
        {
            memmove(to, from, width);
            to += stride;
            from += stride;
        }
    }

    /**
     * 0x004C6B09
     *
     * @param edi @<edi>
     * @param x @<dx>
     * @param y @<bp>
     * @param viewport @<esi>
     */
    void viewportRedrawAfterShift(Window* window, Viewport* viewport, int16_t x, int16_t y)
    {
        if (window != nullptr)
        {
            // skip current window and non-intersecting windows
            if (viewport == window->viewports[0] || viewport == window->viewports[1] || viewport->x + viewport->width <= window->x || viewport->x >= window->x + window->width || viewport->y + viewport->height <= window->y || viewport->y >= window->y + window->height)
            {
                size_t nextWindowIndex = WindowManager::indexOf(window) + 1;
                auto nextWindow = nextWindowIndex >= count() ? nullptr : get(nextWindowIndex);
                viewportRedrawAfterShift(nextWindow, viewport, x, y);
                return;
            }

            // save viewport
            Ui::Viewport view_copy = *viewport;

            if (viewport->x < window->x)
            {
                viewport->width = window->x - viewport->x;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);

                viewport->x += viewport->width;
                viewport->view_x += viewport->width << viewport->zoom;
                viewport->width = view_copy.width - viewport->width;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);
            }
            else if (viewport->x + viewport->width > window->x + window->width)
            {
                viewport->width = window->x + window->width - viewport->x;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);

                viewport->x += viewport->width;
                viewport->view_x += viewport->width << viewport->zoom;
                viewport->width = view_copy.width - viewport->width;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);
            }
            else if (viewport->y < window->y)
            {
                viewport->height = window->y - viewport->y;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);

                viewport->y += viewport->height;
                viewport->view_y += viewport->height << viewport->zoom;
                viewport->height = view_copy.height - viewport->height;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);
            }
            else if (viewport->y + viewport->height > window->y + window->height)
            {
                viewport->height = window->y + window->height - viewport->y;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);

                viewport->y += viewport->height;
                viewport->view_y += viewport->height << viewport->zoom;
                viewport->height = view_copy.height - viewport->height;
                viewport->view_width = viewport->width << viewport->zoom;
                viewportRedrawAfterShift(window, viewport, x, y);
            }

            // restore viewport
            *viewport = view_copy;
        }
        else
        {
            int16_t left = viewport->x;
            int16_t top = viewport->y;
            int16_t right = left + viewport->width;
            int16_t bottom = top + viewport->height;

            // if moved more than the viewport size
            if (std::abs(x) >= viewport->width || std::abs(y) >= viewport->width)
            {
                // redraw whole viewport
                Gfx::redrawScreenRect(left, top, right, bottom);
            }
            else
            {
                // update whole block ?
                copyRect(left, top, viewport->width, viewport->height, x, y);

                if (x > 0)
                {
                    // draw left
                    int16_t _right = left + x;
                    Gfx::redrawScreenRect(left, top, _right, bottom);
                    left += x;
                }
                else if (x < 0)
                {
                    // draw right
                    int16_t _left = right + x;
                    Gfx::redrawScreenRect(_left, top, right, bottom);
                    right += x;
                }

                if (y > 0)
                {
                    // draw top
                    bottom = top + y;
                    Gfx::redrawScreenRect(left, top, right, bottom);
                }
                else if (y < 0)
                {
                    // draw bottom
                    top = bottom + y;
                    Gfx::redrawScreenRect(left, top, right, bottom);
                }
            }
        }
    }

    /**
     * 0x004A0A18
     *
     * @param visibility @<al>
     */
    void viewportSetVisibility(ViewportVisibility visibility)
    {
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
            return;

        auto viewport = window->viewports[0];
        bool flagsChanged = false;

        switch (visibility)
        {
            case ViewportVisibility::undergroundView:
            {
                if (!(viewport->flags & (ViewportFlags::underground_view)))
                {
                    viewport->flags |= (ViewportFlags::underground_view);
                    flagsChanged = true;
                }
                break;
            }

            case ViewportVisibility::heightMarksOnLand:
            {
                if (!(viewport->flags & (ViewportFlags::height_marks_on_land)))
                {
                    viewport->flags |= (ViewportFlags::height_marks_on_land);
                    flagsChanged = true;
                }
                break;
            }

            case ViewportVisibility::overgroundView:
            {
                if ((viewport->flags & (ViewportFlags::underground_view)))
                {
                    viewport->flags &= ~(ViewportFlags::underground_view);
                    flagsChanged = true;
                }
                break;
            }

            default:
            {
                if (viewport->flags & (ViewportFlags::underground_view))
                {
                    viewport->flags &= ~(ViewportFlags::underground_view);
                    flagsChanged = true;
                }

                if (viewport->flags & (ViewportFlags::flag_7))
                {
                    viewport->flags &= ~(ViewportFlags::flag_7);
                    flagsChanged = true;
                }

                if (viewport->flags & (ViewportFlags::flag_8))
                {
                    viewport->flags &= ~(ViewportFlags::flag_8);
                    flagsChanged = true;
                }

                if (viewport->flags & (ViewportFlags::hide_foreground_tracks_roads))
                {
                    viewport->flags &= ~(ViewportFlags::hide_foreground_tracks_roads);
                    flagsChanged = true;
                }

                if (viewport->flags & (ViewportFlags::hide_foreground_scenery_buildings))
                {
                    viewport->flags &= ~(ViewportFlags::hide_foreground_scenery_buildings);
                    flagsChanged = true;
                }

                if (viewport->flags & (ViewportFlags::height_marks_on_land))
                {
                    viewport->flags &= ~(ViewportFlags::height_marks_on_land);
                    flagsChanged = true;
                }

                if (viewport->flags & (ViewportFlags::height_marks_on_tracks_roads))
                {
                    viewport->flags &= ~(ViewportFlags::height_marks_on_tracks_roads);
                    flagsChanged = true;
                }
                break;
            }
        }

        if (flagsChanged)
            window->invalidate();
    }

    // 0x004CF456
    void closeAllFloatingWindows()
    {
        close(WindowType::dropdown, 0);

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (Ui::Window* w = _windowsEnd - 1; w >= _windows; w--)
            {
                if ((w->flags & WindowFlags::stickToBack) != 0)
                    continue;

                if ((w->flags & WindowFlags::stickToFront) != 0)
                    continue;

                close(w);

                // restart loop
                changed = true;
                break;
            }
        }
    }

    int32_t getCurrentRotation()
    {
        return _gCurrentRotation;
    }

    void setCurrentRotation(int32_t value)
    {
        _gCurrentRotation = value;
    }
}

namespace OpenLoco::Ui::Windows
{
    static loco_global<uint8_t, 0x00508F09> suppressErrorSound;
    static loco_global<int8_t, 0x00F2533F> _gridlinesState;
    static loco_global<uint8_t, 0x0112C2E1> _directionArrowsState;

    // 0x00431A8A
    void showError(string_id title, string_id message, bool sound)
    {
        if (!sound)
        {
            suppressErrorSound = true;
        }

        Windows::Error::open(title, message);

        suppressErrorSound = false;
    }

    // 0x00468FD3
    void showGridlines()
    {
        if (!_gridlinesState)
        {
            auto window = WindowManager::getMainWindow();
            if (window != nullptr)
            {
                if (!(window->viewports[0]->flags & ViewportFlags::gridlines_on_landscape))
                {
                    window->invalidate();
                }
                window->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
            }
        }
        _gridlinesState++;
    }

    // 0x00468FFE
    void hideGridlines()
    {
        _gridlinesState--;
        if (!_gridlinesState)
        {
            if (!(Config::get().flags & Config::Flags::gridlinesOnLandscape))
            {
                auto window = WindowManager::getMainWindow();
                if (window != nullptr)
                {
                    if ((window->viewports[0]->flags & ViewportFlags::gridlines_on_landscape) != 0)
                    {
                        window->invalidate();
                    }
                    window->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;
                }
            }
        }
    }

    // 0x004793C4
    void showDirectionArrows()
    {
        if (!_directionArrowsState)
        {
            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                if (!(mainWindow->viewports[0]->flags & ViewportFlags::one_way_direction_arrows))
                {
                    mainWindow->viewports[0]->flags |= ViewportFlags::one_way_direction_arrows;
                    mainWindow->invalidate();
                }
            }
        }
        _directionArrowsState++;
    }

    // 0x004793EF
    void hideDirectionArrows()
    {
        _directionArrowsState--;
        if (!_directionArrowsState)
        {
            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                if ((mainWindow->viewports[0]->flags & ViewportFlags::one_way_direction_arrows))
                {
                    mainWindow->viewports[0]->flags &= ~ViewportFlags::one_way_direction_arrows;
                    mainWindow->invalidate();
                }
            }
        }
    }
}
