#include "WindowManager.h"
#include "../audio/audio.h"
#include "../companymgr.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../map/tile.h"
#include "../multiplayer.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../townmgr.h"
#include "../tutorial.h"
#include "../ui.h"
#include "../viewportmgr.h"
#include "scrollview.h"
#include <algorithm>
#include <cinttypes>
#include <memory>

using namespace openloco::interop;

namespace openloco::ui::WindowManager
{
    namespace find_flag
    {
        constexpr uint16_t by_type = 1 << 7;
    }

    constexpr size_t max_windows = 12;

    static loco_global<uint16_t, 0x0050C19C> time_since_last_tick;
    static loco_global<uint16_t, 0x0052334E> _thousandthTickCounter;
    static loco_global<WindowType, 0x00523364> _callingWindowType;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x00508F10> __508F10;
    static loco_global<gfx::drawpixelinfo_t, 0x0050B884> _screen_dpi;
    static loco_global<uint16_t, 0x0052334E> gWindowUpdateTicks;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIdx;
    static loco_global<uint8_t, 0x005233B6> _currentModalType;
    static loco_global<uint32_t, 0x00523508> _523508;
    static loco_global<int32_t, 0x00525330> _cursorWheel;
    static loco_global<uint32_t, 0x00525E28> _525E28;
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint32_t, 0x009DA3D4> _9DA3D4;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<window[max_windows], 0x011370AC> _windows;
    static loco_global<window*, 0x0113D754> _windowsEnd;

    void sub_4C6B09(window* window, viewport* viewport, int16_t x, int16_t y);

    void init()
    {
        _windowsEnd = &_windows[0];
        _523508 = 0;
    }

    void registerHooks()
    {
        register_hook(
            0x0043DA43,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                windows::LandscapeGeneration::open();
                regs = backup;

                return 0;
            });

        register_hook(
            0x0043EE58,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                windows::ScenarioOptions::open();
                regs = backup;

                return 0;
            });

        register_hook(
            0x0045EFDB,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                window->viewport_zoom_in(false);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045F015,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                window->viewport_zoom_out(false);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045F18B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                callViewportRotateEventOnAllWindows();
                regs = backup;

                return 0;
            });

        register_hook(
            0x00499B7E,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                windows::town::open(regs.dx);
                regs = backup;

                return 0;
            });

        register_hook(
            0x004B93A5,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                sub_4B93A5(regs.bx);
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C5FC8,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto dpi = &addr<0x005233B8, gfx::drawpixelinfo_t>();
                auto window = (ui::window*)regs.esi;

                // Make a copy to prevent overwriting from nested calls
                auto regs2 = regs;

                drawSingle(dpi, window, regs2.ax, regs2.bx, regs2.dx, regs2.bp);
                window++;

                while (window < addr<0x0113D754, ui::window*>())
                {
                    if ((window->flags & ui::window_flags::transparent) != 0)
                    {
                        drawSingle(dpi, window, regs2.ax, regs2.bx, regs2.dx, regs2.bp);
                    }
                    window++;
                }

                return 0;
            });

        register_hook(
            0x004C9984,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidateAllWindowsAfterInput();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C9A95,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                auto window = findAt(regs.ax, regs.bx);
                regs = backup;
                regs.esi = (uintptr_t)window;

                return 0;
            });

        register_hook(
            0x004C9AFA,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                auto window = findAtAlt(regs.ax, regs.bx);
                regs = backup;
                regs.esi = (uintptr_t)window;

                return 0;
            });

        register_hook(
            0x004C9B56,
            [](registers& regs) -> uint8_t {
                ui::window* w;
                if (regs.cx & find_flag::by_type)
                {
                    w = find((WindowType)(regs.cx & ~find_flag::by_type));
                }
                else
                {
                    w = find((WindowType)regs.cx, regs.dx);
                }

                regs.esi = (uintptr_t)w;
                if (w == nullptr)
                {
                    return X86_FLAG_ZERO;
                }

                return 0;
            });

        register_hook(
            0x004CB966,
            [](registers& regs) -> uint8_t {
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

        register_hook(
            0x004CC692,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                if ((regs.cx & find_flag::by_type) != 0)
                {
                    close((WindowType)(regs.cx & ~find_flag::by_type));
                }
                else
                {
                    close((WindowType)regs.cx, regs.dx);
                }
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CC6EA,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                close(window);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004CD296,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                relocateWindows();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CD3D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                dispatchUpdateAll();
                return 0;
            });

        register_hook(
            0x004CE438,
            [](registers& regs) -> uint8_t {
                auto w = getMainWindow();

                regs.esi = (uintptr_t)w;
                if (w == nullptr)
                {
                    return X86_FLAG_CARRY;
                }

                return 0;
            });

        register_hook(
            0x004CEE0B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                sub_4CEE0B((ui::window*)regs.esi);
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C9F5D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto w = createWindow((WindowType)regs.cl, gfx::point_t(regs.ax, regs.eax >> 16), gfx::ui_size_t(regs.bx, regs.ebx >> 16), regs.ecx >> 8, (window_event_list*)regs.edx);
                regs = backup;

                regs.esi = (uintptr_t)w;
                return 0;
            });

        register_hook(
            0x004C9C68,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                auto w = createWindow((WindowType)regs.cl, gfx::ui_size_t(regs.bx, (((uint32_t)regs.ebx) >> 16)), regs.ecx >> 8, (window_event_list*)regs.edx);
                regs = backup;

                regs.esi = (uintptr_t)w;

                return 0;
            });
    }

    window* get(size_t index)
    {
        return &_windows[index];
    }

    size_t count()
    {
        return ((uintptr_t)*_windowsEnd - (uintptr_t)_windows.get()) / sizeof(window);
    }

    WindowType getCurrentModalType()
    {
        return (WindowType)*_currentModalType;
    }

    void setCurrentModalType(WindowType type)
    {
        _currentModalType = (uint8_t)type;
    }

    // 0x004C6118
    void update()
    {
        _tooltipNotShownTicks = _tooltipNotShownTicks + time_since_last_tick;

        if (!ui::dirty_blocks_initialised())
        {
            return;
        }

        if (!intro::is_active())
        {
            gfx::draw_dirty_blocks();
        }

        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            w->viewports_update_position();
        }

        // 1000 tick update
        _thousandthTickCounter = _thousandthTickCounter + time_since_last_tick;
        if (_thousandthTickCounter >= 1000)
        {
            _thousandthTickCounter = 0;
            for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
            {
                w->call_on_periodic_update();
            }
        }

        // Border flash invalidation
        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if ((w->flags & window_flags::white_border_mask) != 0)
            {
                // TODO: Replace with countdown
                w->flags -= window_flags::white_border_one;
                if ((w->flags & window_flags::white_border_mask) == 0)
                {
                    w->invalidate();
                }
            }
        }

        allWheelInput();
    }

    // 0x004CE438
    window* getMainWindow()
    {
        return find(WindowType::main);
    }

    // 0x004C9B56
    window* find(WindowType type)
    {
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type == type)
            {
                return w;
            }
        }

        return nullptr;
    }

    // 0x004C9B56
    window* find(WindowType type, window_number number)
    {
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type == type && w->number == number)
            {
                return w;
            }
        }

        return nullptr;
    }

    // 0x004C9A95
    window* findAt(int16_t x, int16_t y)
    {
        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (x < w->x)
                continue;

            if (x >= (w->x + w->width))
                continue;

            if (y < w->y)
                continue;
            if (y >= (w->y + w->height))
                continue;

            if ((w->flags & window_flags::flag_7) != 0)
                continue;

            if ((w->flags & window_flags::no_background) != 0)
            {
                auto index = w->find_widget_at(x, y);
                if (index == -1)
                {
                    continue;
                }
            }

            if (w->call_on_resize() == nullptr)
            {
                return findAt(x, y);
            }

            return w;
        }

        return nullptr;
    }

    window* findAt(gfx::point_t point)
    {
        return findAt(point.x, point.y);
    }

    // 0x004C9AFA
    window* findAtAlt(int16_t x, int16_t y)
    {
        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (x < w->x)
                continue;

            if (x >= (w->x + w->width))
                continue;

            if (y < w->y)
                continue;
            if (y >= (w->y + w->height))
                continue;

            if ((w->flags & window_flags::no_background) != 0)
            {
                auto index = w->find_widget_at(x, y);
                if (index == -1)
                {
                    continue;
                }
            }

            if (w->call_on_resize() == nullptr)
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
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != type)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidate(WindowType type, window_number number)
    {
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidateWidget(WindowType type, window_number number, uint8_t widget_index)
    {
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            auto widget = w->widgets[widget_index];

            if (widget.left != -2)
            {
                gfx::set_dirty_blocks(
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
        if (is_paused())
        {
            _523508++;
        }

        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            w->update_scroll_widgets();
            w->invalidate_pressed_image_buttons();
            w->call_on_resize();
        }
    }

    // 0x004CC692
    void close(WindowType type)
    {
        bool repeat = true;
        while (repeat)
        {
            repeat = false;
            for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
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
    void close(WindowType type, window_number id)
    {
        auto window = find(type, id);
        if (window != nullptr)
        {
            close(window);
        }
    }

    // 0x004CC750
    window* bringToFront(window* w)
    {
        registers regs;
        regs.esi = (uint32_t)w;
        call(0x004CC750, regs);

        return (window*)regs.esi;
    }

    // 0x004CD3A9
    window* bringToFront(WindowType type, uint16_t id)
    {
        registers regs;
        regs.cx = (uint8_t)type;
        regs.dx = id;
        call(0x004CD3A9, regs);

        return (window*)regs.esi;
    }

    /**
     * 0x004C9BEA
     *
     * @param x @<dx>
     * @param y @<ax>
     * @param width @<bx>
     * @param height @<cx>
     */
    static bool window_fits_within_space(gfx::point_t position, gfx::ui_size_t size)
    {
        if (position.x < 0)
            return false;

        if (position.y < 28)
            return false;

        if (position.x + size.width > ui::width())
            return false;

        if (position.y + size.height > ui::width())
            return false;

        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if ((w->flags & window_flags::stick_to_back) != 0)
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
    static window* createWindowOnScreen(
        WindowType type,
        gfx::point_t origin,
        gfx::ui_size_t size,
        uint32_t flags,
        window_event_list* events)
    {
        origin.x = std::clamp<decltype(origin.x)>(origin.x, 0, ui::width() - size.width);

        return createWindow(type, origin, size, flags, events);
    }

    // 0x004C9BA2
    static bool window_fits_on_screen(gfx::point_t origin, gfx::ui_size_t size)
    {
        if (origin.x < -(size.width / 4))
            return false;
        if (origin.x > ui::width() - (size.width / 2))
            return false;

        if (origin.y < 28)
            return false;
        if (origin.y > ui::height() - (size.height / 4))
            return false;

        return window_fits_within_space(origin, size);
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
    window* createWindow(
        WindowType type,
        gfx::ui_size_t size,
        uint32_t flags,
        window_event_list* events)
    {
        gfx::point_t position{};

        position.x = 0;  // dx
        position.y = 30; // ax
        if (window_fits_within_space(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        position.x = ui::width() - size.width;
        position.y = 30;
        if (window_fits_within_space(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        position.x = 0;
        position.y = ui::height() - size.height - 29;
        if (window_fits_within_space(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        position.x = ui::width() - size.width;
        position.y = ui::height() - size.height - 29;
        if (window_fits_within_space(position, size))
            return createWindowOnScreen(type, position, size, flags, events);

        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->flags & window_flags::stick_to_back)
                continue;

            position.x = w->x + w->width + 2;
            position.y = w->y;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x - size.width - 2;
            position.y = w->y;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y + w->height + 2;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y - size.height - 2;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x + w->width + 2;
            position.y = w->y + w->height - size.height;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x - size.width - 2;
            position.y = w->y + w->height - size.height;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x + w->width - size.width;
            position.y = w->y - size.height - 2;
            if (window_fits_within_space(position, size))
                return createWindowOnScreen(type, position, size, flags, events);
        }

        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->flags & window_flags::stick_to_back)
                continue;

            position.x = w->x + w->width + 2;
            position.y = w->y;
            if (window_fits_on_screen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x - size.width - 2;
            position.y = w->y;
            if (window_fits_on_screen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y + w->height + 2;
            if (window_fits_on_screen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);

            position.x = w->x;
            position.y = w->y - size.height - 2;
            if (window_fits_on_screen(position, size))
                return createWindowOnScreen(type, position, size, flags, events);
        }

        position.x = 0;
        position.y = 30;

        bool loop;
        do
        {
            loop = false;
            for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
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
    window* createWindow(
        WindowType type,
        gfx::point_t origin,
        gfx::ui_size_t size,
        uint32_t flags,
        window_event_list* events)
    {
        if (count() == max_windows)
        {
            for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
            {
                if ((w->flags & window_flags::stick_to_back) != 0)
                    continue;

                if ((w->flags & window_flags::stick_to_front) != 0)
                    continue;

                if ((w->flags & window_flags::no_auto_close) != 0)
                    continue;

                close(w);
                return createWindow(type, origin, size, flags, events);
            }
        }

        bool stickToBack = (flags & window_flags::stick_to_back) != 0;
        bool stickToFront = (flags & window_flags::stick_to_front) != 0;
        bool hasFlag12 = (flags & window_flags::flag_12) != 0;
        bool hasFlag13 = (flags & window_flags::flag_13) != 0;

        // Find right position to insert new window
        size_t dstIndex = 0;
        if (stickToBack)
        {
            for (size_t i = 0; i < count(); i++)
            {
                if ((_windows[i].flags & window_flags::stick_to_back) != 0)
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
                if ((_windows[i - 1].flags & window_flags::stick_to_front) == 0)
                {
                    dstIndex = i;
                    break;
                }
            }
        }

        auto window = ui::window(origin, size);
        window.type = type;
        window.flags = flags;
        if (hasFlag12 || (!stickToBack && !stickToFront && !hasFlag13))
        {
            window.flags |= window_flags::white_border_mask;
            audio::play_sound(audio::sound_id::open_window, origin.x + size.width / 2);
        }

        window.event_handlers = events;

        size_t length = _windowsEnd - (_windows + dstIndex);
        memmove(_windows + dstIndex + 1, _windows + dstIndex, length * sizeof(ui::window));
        _windowsEnd = _windowsEnd + 1;
        _windows[dstIndex] = window;

        _windows[dstIndex].invalidate();

        return &_windows[dstIndex];
    }

    window* createWindowCentred(WindowType type, gfx::ui_size_t size, uint32_t flags, window_event_list* events)
    {
        auto x = (ui::width() / 2) - (size.width / 2);
        auto y = std::max(28, (ui::height() / 2) - (size.height / 2));
        return createWindow(type, gfx::point_t(x, y), size, flags, events);
    }

    // 0x004C5FC8
    void drawSingle(gfx::drawpixelinfo_t* _dpi, window* w, int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        // Copy dpi so we can crop it
        auto dpi = *_dpi;

        // Clamp left to 0
        int32_t overflow = left - dpi.x;
        if (overflow > 0)
        {
            dpi.x += overflow;
            dpi.width -= overflow;
            if (dpi.width <= 0)
                return;
            dpi.pitch += overflow;
            dpi.bits += overflow;
        }

        // Clamp width to right
        overflow = dpi.x + dpi.width - right;
        if (overflow > 0)
        {
            dpi.width -= overflow;
            if (dpi.width <= 0)
                return;
            dpi.pitch += overflow;
        }

        // Clamp top to 0
        overflow = top - dpi.y;
        if (overflow > 0)
        {
            dpi.y += overflow;
            dpi.height -= overflow;
            if (dpi.height <= 0)
                return;
            dpi.bits += (dpi.width + dpi.pitch) * overflow;
        }

        // Clamp height to bottom
        overflow = dpi.y + dpi.height - bottom;
        if (overflow > 0)
        {
            dpi.height -= overflow;
            if (dpi.height <= 0)
                return;
        }

        if (is_unknown_4_mode() && w->type != WindowType::wt_47)
        {
            return;
        }

        // Company colour
        if (w->owner != company_id::null)
        {
            w->colours[0] = companymgr::get_company_colour(w->owner);
        }

        addr<0x1136F9C, int16_t>() = w->x;
        addr<0x1136F9E, int16_t>() = w->y;

        loco_global<uint8_t[4], 0x1136594> windowColours;
        // Text colouring
        windowColours[0] = colour::opaque(w->colours[0]);
        windowColours[1] = colour::opaque(w->colours[1]);
        windowColours[2] = colour::opaque(w->colours[2]);
        windowColours[3] = colour::opaque(w->colours[3]);

        w->call_prepare_draw();
        w->call_draw(&dpi);
    }

    // 0x004CD3D0
    void dispatchUpdateAll()
    {
        _523508++;
        companymgr::updating_company_id(companymgr::get_controlling_id());

        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            w->call_update();
        }

        ui::textinput::sub_4CE6FF();
        call(0x4CEEA7);
    }

    // 0x004CC6EA
    void close(window* window)
    {
        if (window == nullptr)
        {
            return;
        }

        // Make a copy of the window class and number in case
        // the window order is changed by the close event.
        auto type = window->type;
        uint16_t number = window->number;

        window->call_close();

        window = find(type, number);
        if (window == nullptr)
            return;

        if (window->viewports[0] != nullptr)
        {
            window->viewports[0]->width = 0;
            window->viewports[0] = nullptr;
        }

        if (window->viewports[1] != nullptr)
        {
            window->viewports[1]->width = 0;
            window->viewports[1] = nullptr;
        }

        window->invalidate();

        // Remove window from list and reshift all windows
        _windowsEnd--;
        int windowCount = *_windowsEnd - window;
        if (windowCount > 0)
        {
            memmove(window, window + 1, windowCount * sizeof(ui::window));
        }

        viewportmgr::collectGarbage();
    }

    void callEvent8OnAllWindows()
    {
        for (window* window = _windowsEnd - 1; window >= _windows; window--)
        {
            window->call_8();
        }
    }

    void callEvent9OnAllWindows()
    {
        for (window* window = _windowsEnd - 1; window >= _windows; window--)
        {
            window->call_9();
        }
    }

    // 0x0045F18B
    void callViewportRotateEventOnAllWindows()
    {
        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            w->call_viewport_rotate();
        }
    }

    // 0x004CD296
    void relocateWindows()
    {
        int16_t newLocation = 8;
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            // Work out if the window requires moving
            bool extendsX = (w->x + 10) >= ui::width();
            bool extendsY = (w->y + 10) >= ui::height();
            if ((w->flags & window_flags::stick_to_back) != 0 || (w->flags & window_flags::stick_to_front) != 0)
            {
                // toolbars are 27px high
                extendsY = (w->y + 10 - 27) >= ui::height();
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
    void sub_4CEE0B(window* self)
    {
        int left = self->x;
        int right = self->x + self->width;
        int top = self->y;
        int bottom = self->y + self->height;

        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w == self)
                continue;

            if (w->flags & window_flags::stick_to_back)
                continue;

            if (w->flags & window_flags::stick_to_front)
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

            if (bottom < ui::height() - 80)
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
    void sub_4B93A5(window_number number)
    {
        for (ui::window* w = &_windows[0]; w != _windowsEnd; w++)
        {
            if (w->type != WindowType::vehicle)
                continue;

            if (w->number != number)
                continue;

            if (w->current_tab != 4)
                continue;

            w->invalidate();
        }
    }

    // 0x004A0AB0
    void closeConstructionWindows()
    {
        close(WindowType::construction);
        close(WindowType::companyFaceSelection);
        input::cancel_tool();
        addr<0x00522096, uint8_t>() = 0;
    }

    // 0x004BF089
    void closeTopmost()
    {
        close(WindowType::dropdown, 0);

        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (w->flags & window_flags::stick_to_back)
                continue;

            if (w->flags & window_flags::stick_to_front)
                continue;

            close(w);
            break;
        }
    }

    static void windowScrollWheelInput(ui::window* window, widget_index widgetIndex, int wheel)
    {
        int scrollIndex = window->get_scroll_data_index(widgetIndex);
        scroll_area_t* scroll = &window->scroll_areas[scrollIndex];
        ui::widget_t* widget = &window->widgets[widgetIndex];

        if (window->scroll_areas[scrollIndex].flags & 0b10000)
        {
            int size = widget->bottom - widget->top - 1;
            if (scroll->flags & 0b1)
                size -= 11;
            size = std::max(0, scroll->v_bottom - size);
            scroll->v_top = std::clamp(scroll->v_top + wheel, 0, size);
        }
        else if (window->scroll_areas[scrollIndex].flags & 0b1)
        {
            int size = widget->right - widget->left - 1;
            if (scroll->flags & 0b10000)
                size -= 11;
            size = std::max(0, scroll->h_right - size);
            scroll->h_left = std::clamp(scroll->h_left + wheel, 0, size);
        }

        ui::scrollview::update_thumbs(window, widgetIndex);
        invalidateWidget(window->type, window->number, widgetIndex);
    }

    // 0x004C628E
    static bool windowWheelInput(window* window, int wheel)
    {
        int widgetIndex = -1;
        int scrollIndex = -1;
        for (widget_t* widget = window->widgets; widget->type != widget_type::end; widget++)
        {
            widgetIndex++;

            if (widget->type != widget_type::scrollview)
                continue;

            scrollIndex++;
            if (window->scroll_areas[scrollIndex].flags & 0b10001)
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

        if (tutorial::state() != tutorial::tutorial_state::none)
            return;

        if (input::has_flag(input::input_flags::flag5))
        {
            if (openloco::is_title_mode())
                return;

            auto main = WindowManager::getMainWindow();
            if (main != nullptr)
            {
                if (wheel > 0)
                {
                    main->viewport_rotate_right();
                }
                else if (wheel < 0)
                {
                    main->viewport_rotate_left();
                }
                townmgr::update_labels();
                stationmgr::update_labels();
                windows::map::center_on_view_point();
            }

            return;
        }

        const gfx::point_t cursorPosition = input::getMouseLocation();
        auto window = findAt(cursorPosition);

        if (window != nullptr)
        {
            if (window->type == WindowType::main)
            {
                if (openloco::is_title_mode())
                    return;

                if (wheel > 0)
                {
                    window->viewport_zoom_out(true);
                }
                else if (wheel < 0)
                {
                    window->viewport_zoom_in(true);
                }
                townmgr::update_labels();
                stationmgr::update_labels();

                return;
            }
            else
            {
                auto widgetIndex = window->find_widget_at(cursorPosition.x, cursorPosition.y);
                if (widgetIndex != -1)
                {
                    if (window->widgets[widgetIndex].type == widget_type::scrollview)
                    {
                        auto scrollIndex = window->get_scroll_data_index(widgetIndex);
                        if (window->scroll_areas[scrollIndex].flags & 0b10001)
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

        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (windowWheelInput(w, wheel))
            {
                return;
            }
        }
    }

    bool isInFront(ui::window* window)
    {
        for (ui::window* w = window + 1; w != _windowsEnd; w++)
        {
            if ((w->flags & window_flags::stick_to_front) != 0)
                continue;

            return false;
        }

        return true;
    }

    bool isInFrontAlt(ui::window* window)
    {
        for (ui::window* w = window + 1; w != _windowsEnd; w++)
        {
            if ((w->flags & window_flags::stick_to_front) != 0)
                continue;

            if (w->type == WindowType::buildVehicle)
                continue;

            return false;
        }

        return true;
    }

    // 0x0046960C
    ui::window* findWindowShowing(map::map_pos position)
    {
        for (ui::window* w = _windowsEnd - 1; w >= _windows; w--)
        {
            if (w->viewports[0] == nullptr)
                continue;

            auto viewport = w->viewports[0];
            if (viewport->zoom != 0)
                continue;

            if (!viewport->contains({ position.x, position.y }))
                continue;

            return w;
        }

        return nullptr;
    }
}

namespace openloco::ui::windows
{
    static loco_global<uint8_t, 0x00508F09> suppressErrorSound;

    // 0x00431A8A
    void show_error(string_id title, string_id message, bool sound)
    {
        if (!sound)
        {
            suppressErrorSound = true;
        }

        registers regs;
        regs.bx = (uint16_t)title;
        regs.dx = (uint16_t)message;
        call(0x00431A8A, regs);

        suppressErrorSound = false;
    }
}
