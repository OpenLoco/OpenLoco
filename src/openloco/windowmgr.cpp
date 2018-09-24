#include "windowmgr.h"
#include "companymgr.h"
#include "console.h"
#include "graphics/colours.h"
#include "input.h"
#include "interop/interop.hpp"
#include "things/thing.h"
#include "things/thingmgr.h"
#include "tutorial.h"
#include "ui.h"
#include "ui/scrollview.h"
#include <algorithm>

using namespace openloco::interop;

namespace openloco::ui::windowmgr
{
    namespace find_flag
    {
        constexpr uint16_t by_type = 1 << 7;
    }

    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;
    static loco_global<ui::window_type, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIdx;
    loco_global<uint8_t, 0x005233B6> _current_modal_type;
    loco_global<uint32_t, 0x00523508> _523508;
    loco_global<window[12], 0x011370AC> _windows;
    loco_global<window*, 0x0113D754> _windows_end;

    struct WindowList
    {
        window* begin() const { return &_windows[0]; };
        window* end() const
        {
            if (_windows_end)
                return _windows_end;
            else
                return &_windows[0];
        };
    };

    static void sub_4B92A5(ui::window* window);

    void init()
    {
        _windows_end = &_windows[0];
        _523508 = 0;
    }

    void register_hooks()
    {
        register_hook(
            0x0045EFDB,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                window->viewport_zoom_out(false);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045F015,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                window->viewport_zoom_in(false);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045F18B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                call_event_viewport_rotate_on_all_windows();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004B92A5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4B92A5((ui::window*)regs.esi);
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
            0x004BF089,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                close_topmost();
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

                draw_single(dpi, window, regs2.ax, regs2.bx, regs2.dx, regs2.bp);
                window++;

                while (window < addr<0x0113D754, ui::window*>())
                {
                    if ((window->flags & ui::window_flags::transparent) != 0)
                    {
                        draw_single(dpi, window, regs2.ax, regs2.bx, regs2.dx, regs2.bp);
                    }
                    window++;
                }

                return 0;
            });

        register_hook(
            0x004C6202,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                all_wheel_input();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C9984,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidate_all_windows_after_input();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C9A95,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                auto window = find_at(regs.ax, regs.bx);
                regs = backup;
                regs.esi = (uintptr_t)window;

                return 0;
            });

        register_hook(
            0x004C9AFA,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                auto window = find_at_alt(regs.ax, regs.bx);
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
                    w = find((ui::window_type)(regs.cx & ~find_flag::by_type));
                }
                else
                {
                    w = find((ui::window_type)regs.cx, regs.dx);
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
                    invalidate_widget((ui::window_type)(regs.al & 0x7F), regs.bx, regs.ah);
                }
                else if ((regs.al & 1 << 6) != 0)
                {
                    invalidate((ui::window_type)(regs.al & 0xBF));
                }
                else
                {
                    invalidate((ui::window_type)regs.al, regs.bx);
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
                    close((ui::window_type)(regs.cx & ~find_flag::by_type));
                }
                else
                {
                    close((ui::window_type)regs.cx, regs.dx);
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
                relocate_windows();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CD3D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                dispatch_update_all();
                return 0;
            });

        register_hook(
            0x004CE438,
            [](registers& regs) -> uint8_t {
                auto w = get_main();

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
    }

    window* get(size_t index)
    {
        return &_windows[index];
    }

    size_t num_windows()
    {
        return ((uintptr_t)*_windows_end - (uintptr_t)_windows.get()) / sizeof(window);
    }

    window_type current_modal_type()
    {
        return (window_type)*_current_modal_type;
    }

    void current_modal_type(window_type type)
    {
        _current_modal_type = (uint8_t)type;
    }

    // 0x004C6118
    void update()
    {
        call(0x004C6118);
    }

    // 0x004CE438
    window* get_main()
    {
        return find(window_type::main);
    }

    // 0x004C9B56
    window* find(window_type type)
    {
        for (window& w : WindowList())
        {
            if (w.type == type)
            {
                return &w;
            }
        }

        return nullptr;
    }

    // 0x004C9B56
    window* find(window_type type, window_number number)
    {
        for (window& w : WindowList())
        {
            if (w.type == type && w.number == number)
            {
                return &w;
            }
        }

        return nullptr;
    }

    // 0x004C9A95
    window* find_at(int16_t x, int16_t y)
    {
        window* w = _windows_end;
        while (w > _windows)
        {
            w--;
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
                return find_at(x, y);
            }

            return w;
        }

        return nullptr;
    }

    // 0x004C9AFA
    window* find_at_alt(int16_t x, int16_t y)
    {
        window* w = _windows_end;
        while (w > _windows)
        {
            w--;

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
                return find_at_alt(x, y);
            }

            return w;
        }

        return nullptr;
    }

    // 0x004CB966
    void invalidate(window_type type)
    {
        for (window& w : WindowList())
        {
            if (w.type != type)
                continue;

            w.invalidate();
        }
    }

    // 0x004CB966
    void invalidate(window_type type, window_number number)
    {
        for (window& w : WindowList())
        {
            if (w.type != type)
                continue;

            if (w.number != number)
                continue;

            w.invalidate();
        }
    }

    // 0x004CB966
    void invalidate_widget(window_type type, window_number number, uint8_t widget_index)
    {
        for (window& w : WindowList())
        {
            if (w.type != type)
                continue;

            if (w.number != number)
                continue;

            auto widget = w.widgets[widget_index];

            if (widget.left != -2)
            {
                gfx::set_dirty_blocks(
                    w.x + widget.left,
                    w.y + widget.top,
                    w.x + widget.right + 1,
                    w.y + widget.bottom + 1);
            }
        }
    }

    // 0x004C9984
    void invalidate_all_windows_after_input()
    {
        if (is_paused())
        {
            _523508++;
        }

        auto window = *_windows_end;
        while (window > _windows)
        {
            window--;
            window->update_scroll_widgets();
            window->invalidate_pressed_image_buttons();
            window->call_on_resize();
        }
    }

    // 0x004CC692
    void close(window_type type)
    {
        bool repeat = true;
        while (repeat)
        {
            repeat = false;
            for (window& w : WindowList())
            {
                if (w.type != type)
                    continue;

                close(&w);
                repeat = true;
                break;
            }
        }
    }

    // 0x004CC692
    void close(window_type type, window_number id)
    {
        auto window = find(type, id);
        if (window != nullptr)
        {
            close(window);
        }
    }

    // 0x004CC750
    window* bring_to_front(window* w)
    {
        registers regs;
        regs.esi = (uint32_t)w;
        call(0x004CC750, regs);

        return (window*)regs.esi;
    }

    // 0x004CD3A9
    window* bring_to_front(window_type type, uint16_t id)
    {
        registers regs;
        regs.cx = (uint8_t)type;
        regs.dx = id;
        call(0x004CD3A9, regs);

        return (window*)regs.esi;
    }

    // 0x004C9F5D
    window* create_window(
        window_type type,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height,
        int32_t flags,
        window_event_list* events)
    {
        registers regs;
        regs.eax = (y << 16) | (x & 0xFFFF);
        regs.ebx = (height << 16) | (width & 0xFFFF);
        regs.ecx = (uint8_t)type | (flags << 8);
        regs.edx = (int32_t)events;
        call(0x004C9F5D, regs);
        return (window*)regs.esi;
    }

    window* create_window_centred(window_type type, int32_t width, int32_t height, int32_t flags, window_event_list* events)
    {
        auto x = (ui::width() / 2) - (width / 2);
        auto y = std::max(28, (ui::height() / 2) - (height / 2));
        return create_window(type, x, y, width, height, flags, events);
    }

    // 0x004C5FC8
    void draw_single(gfx::drawpixelinfo_t* _dpi, window* w, int32_t left, int32_t top, int32_t right, int32_t bottom)
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

        if (is_unknown_4_mode() && w->type != window_type::wt_47)
        {
            return;
        }

        loco_global<uint8_t[32], 0x9C645C> byte9C645C;

        // Company colour?
        if (w->var_884 != -1)
        {
            w->colours[0] = byte9C645C[w->var_884];
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

    // 0x004C6EE6
    static input::mouse_button game_get_next_input(uint32_t* x, int16_t* y)
    {
        registers regs;
        call(0x004c6ee6, regs);

        *x = regs.eax;
        *y = regs.bx;

        return (input::mouse_button)regs.cx;
    }

    // 0x004CD422
    static void process_mouse_tool(int16_t x, int16_t y)
    {
        if (!input::has_flag(input::input_flags::tool_active))
        {
            return;
        }

        auto window = find(_toolWindowType, _toolWindowNumber);
        if (window != nullptr)
        {
            window->call_tool_update(_toolWidgetIdx, x, y);
        }
        else
        {
            input::cancel_tool();
        }
    }

    // 0x004C98CF
    void sub_4C98CF()
    {
        ui::window* window;

        window = *_windows_end;
        while (window > _windows)
        {
            window--;
            window->call_8();
        }

        invalidate_all_windows_after_input();
        call(0x004c6e65); // update_cursor_position

        uint32_t x;
        int16_t y;
        input::mouse_button state;
        while ((state = game_get_next_input(&x, &y)) != input::mouse_button::released)
        {
            input::handle_mouse(x, y, state);
        }

        if (input::has_flag(input::input_flags::flag5))
        {
            input::handle_mouse(x, y, state);
        }
        else if (x != 0x80000000)
        {
            x = std::clamp<int16_t>(x, 0, ui::width() - 1);
            y = std::clamp<int16_t>(y, 0, ui::height() - 1);

            input::handle_mouse(x, y, state);
            input::process_mouse_over(x, y);
            process_mouse_tool(x, y);
        }

        window = *_windows_end;
        while (window > _windows)
        {
            window--;
            window->call_9();
        }
    }

    // 0x004CD3D0
    void dispatch_update_all()
    {
        _523508++;
        companymgr::updating_company_id(companymgr::get_controlling_id());

        for (ui::window* w = _windows_end - 1; w >= _windows; w--)
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
        _windows_end--;
        int windowCount = *_windows_end - window;
        if (windowCount > 0)
        {
            memmove(window, window + 1, windowCount * sizeof(ui::window));
        }

        call(0x004CEC25); // viewport_update_pointers
    }

    // 0x0045F18B
    void call_event_viewport_rotate_on_all_windows()
    {
        window* w = _windows_end;
        while (w > _windows)
        {
            w--;
            w->call_viewport_rotate();
        }
    }

    // 0x004CD296
    void relocate_windows()
    {
        int16_t newLocation = 8;
        for (window& w : WindowList())
        {
            // Work out if the window requires moving
            bool extendsX = (w.x + 10) >= ui::width();
            bool extendsY = (w.y + 10) >= ui::height();
            if ((w.flags & window_flags::stick_to_back) != 0 || (w.flags & window_flags::stick_to_front) != 0)
            {
                // toolbars are 27px high
                extendsY = (w.y + 10 - 27) >= ui::height();
            }

            if (extendsX || extendsY)
            {
                // Calculate the new locations
                int16_t oldX = w.x;
                int16_t oldY = w.y;
                w.x = newLocation;
                w.y = newLocation + 28;

                // Move the next new location so windows are not directly on top
                newLocation += 8;

                // Adjust the viewports if required.
                if (w.viewports[0] != nullptr)
                {
                    w.viewports[0]->x -= oldX - w.x;
                    w.viewports[0]->y -= oldY - w.y;
                }

                if (w.viewports[1] != nullptr)
                {
                    w.viewports[1]->x -= oldX - w.x;
                    w.viewports[1]->y -= oldY - w.y;
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

        for (window& w : WindowList())
        {
            if (&w == self)
                continue;

            if (w.flags & window_flags::stick_to_back)
                continue;

            if (w.flags & window_flags::stick_to_front)
                continue;

            if (w.x >= right)
                continue;

            if (w.x + w.width <= left)
                continue;

            if (w.y >= bottom)
                continue;

            if (w.y + w.height <= top)
                continue;

            w.invalidate();

            if (bottom < ui::height() - 80)
            {
                int dY = bottom + 3 - w.y;
                w.y += dY;
                w.invalidate();

                if (w.viewports[0] != nullptr)
                {
                    w.viewports[0]->y += dY;
                }

                if (w.viewports[1] != nullptr)
                {
                    w.viewports[1]->y += dY;
                }
            }
        }
    }

    static loco_global<int16_t, 0x01136268> _1136268;
    static loco_global<uint16_t[1], 0x0113626A> _113626A;
    static loco_global<int8_t[1], 0x011364F0> _11364F0;
    static loco_global<int32_t, 0x011364E8> _11364E8;

    static void sub_4B9165(uint8_t dl, uint8_t dh, void* esi)
    {
        registers regs;
        regs.dl = dl;
        regs.dh = dh;
        regs.esi = (uintptr_t)esi;
        if (esi == nullptr)
        {
            regs.esi = -1;
        }

        call(0x4B9165, regs);
    }

    /**
     * 0x004B92A5
     *
     * @param window @<esi>
     */
    static void sub_4B92A5(ui::window* window)
    {
        ui::window* w = _windows_end;
        while (true)
        {
            w--;

            if (w < _windows)
            {
                if (_11364E8 != -1)
                {
                    _11364E8 = -1;
                    window->var_83C = 0;
                    window->invalidate();
                }
                break;
            }

            if (w->type != window_type::vehicle)
                continue;

            if (w->current_tab != 1)
                continue;

            auto vehicle = thingmgr::get<openloco::vehicle>(w->number);
            if (vehicle->var_21 != companymgr::get_controlling_id())
                continue;

            if (_11364E8 != w->number)
            {
                _11364E8 = w->number;
                window->var_83C = 0;
                window->invalidate();
                break;
            }
        }

        uint8_t dl = window->current_tab;
        uint8_t dh = _11364F0[window->var_874];

        thing_base* thing = nullptr;
        if (_11364E8 != -1)
        {
            thing = thingmgr::get<thing_base>(_11364E8);
        }

        sub_4B9165(dl, dh, thing);

        int cx = _1136268;
        if (window->var_83C == cx)
            return;

        uint16_t* src = _113626A;
        uint16_t* dest = (uint16_t*)window->pad_6A;
        window->var_83A = 0;
        while (cx != 0)
        {
            *dest = *src;
            dest++;
            src++;
            cx--;
        }
        window->var_840 = 0xFFFF;
        window->invalidate();
    }

    // 0x004B93A5
    void sub_4B93A5(window_number number)
    {
        for (window& w : WindowList())
        {
            if (w.type != window_type::vehicle)
                continue;

            if (w.number != number)
                continue;

            if (w.current_tab != 4)
                continue;

            w.invalidate();
        }
    }

    // 0x004BF089
    void close_topmost()
    {
        close(window_type::dropdown, 0);

        for (window& w : WindowList())
        {
            if (w.flags & window_flags::stick_to_back)
                continue;

            if (w.flags & window_flags::stick_to_front)
                continue;

            close(&w);
            break;
        }
    }

    static loco_global<int32_t, 0x00525330> _cursorWheel;

    static void window_scroll_wheel_input(ui::window* window, widget_index widgetIndex, int wheel)
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
        invalidate_widget(window->type, window->number, widgetIndex);
    }

    // 0x004C628E
    static bool window_wheel_input(window* window, int wheel)
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
                window_scroll_wheel_input(window, widgetIndex, wheel);
                return true;
            }
        }

        return false;
    }

    // TODO: Move
    // 0x0049771C
    static void sub_49771C()
    {
        // Might have something to do with town labels
        call(0x0049771C);
    }

    // TODO: Move
    // 0x0048DDC3
    static void sub_48DDC3()
    {
        // Might have something to do with station labels
        call(0x0048DDC3);
    }

    // 0x004C6202
    void all_wheel_input()
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

            auto main = windowmgr::get_main();
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
                sub_49771C();
                sub_48DDC3();
                windows::map_center_on_view_point();
            }

            return;
        }

        int32_t x = addr<0x0113E72C, int32_t>();
        int32_t y = addr<0x0113E730, int32_t>();
        auto window = find_at(x, y);

        if (window != nullptr)
        {
            if (window->type == window_type::main)
            {
                if (openloco::is_title_mode())
                    return;

                if (wheel > 0)
                {
                    window->viewport_zoom_in(true);
                }
                else if (wheel < 0)
                {
                    window->viewport_zoom_out(true);
                }
                sub_49771C();
                sub_48DDC3();

                return;
            }
            else
            {
                auto widgetIndex = window->find_widget_at(x, y);
                if (widgetIndex != -1)
                {
                    if (window->widgets[widgetIndex].type == widget_type::scrollview)
                    {
                        auto scrollIndex = window->get_scroll_data_index(widgetIndex);
                        if (window->scroll_areas[scrollIndex].flags & 0b10001)
                        {
                            window_scroll_wheel_input(window, widgetIndex, wheel);
                            return;
                        }
                    }

                    if (window_wheel_input(window, wheel))
                    {
                        return;
                    }
                }
            }
        }

        for (ui::window* w = _windows_end - 1; w >= _windows; w--)
        {
            if (window_wheel_input(w, wheel))
            {
                return;
            }
        }
    }

    bool is_in_front(ui::window* w)
    {
        ui::window* window = w;

        while (true)
        {
            window++;
            if (window >= _windows_end)
                return true;

            if ((window->flags & window_flags::stick_to_front) != 0)
                continue;

            return false;
        }

        return false;
    }

    bool is_in_front_alt(ui::window* w)
    {
        ui::window* window = w;

        while (true)
        {
            window++;
            if (window >= _windows_end)
                return true;

            if ((window->flags & window_flags::stick_to_front) != 0)
                continue;

            if (window->type == window_type::build_vehicle)
                continue;

            return false;
        }

        return false;
    }
}
