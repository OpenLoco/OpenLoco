#include "windowmgr.h"
#include "companymgr.h"
#include "console.h"
#include "graphics/colours.h"
#include "interop/interop.hpp"
#include "ui.h"
#include <algorithm>

using namespace openloco::interop;

namespace openloco::ui::windowmgr
{
    namespace find_flag
    {
        constexpr uint16_t by_type = 1 << 7;
    }

    class Container
    {
        window* _start;
        window* _end;

        struct iterator
        {
            window* e;

            // this is the important one
            window* operator*() { return e; }

            // the rest are just boilerplate
            iterator& operator++()
            {
                ++e;
                return *this;
            }
            iterator operator++(int)
            {
                iterator tmp{ e };
                ++*this;
                return tmp;
            }

            bool operator==(iterator rhs) const { return e == rhs.e; }
            bool operator!=(iterator rhs) const { return e != rhs.e; }
        };

    public:
        iterator begin() { return { _start }; };
        iterator end() { return { _end }; };
        Container(window* start, window* end)
        {
            _start = start;
            _end = end;
        }
    };

    loco_global<uint8_t, 0x005233B6> _current_modal_type;
    loco_global<uint32_t, 0x00523508> _523508;
    loco_global<window[12], 0x011370AC> _windows;
    loco_global<window*, 0x0113D754> _windows_end;

    void register_hooks()
    {
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
            0x004CD3D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                dispatch_update_all();
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
            0x004C9B56,
            [](registers& regs) -> uint8_t {
                ui::window* w;
                if (regs.cx & find_flag::by_type)
                {
                    w = find((ui::window_type)regs.cx);
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
            0x0045F18B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                sub_45F18B();
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
            0x004CEE0B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                sub_4CEE0B((ui::window*)regs.esi);
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
    }

    window* get(size_t index)
    {
        return &_windows.get()[index];
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
        auto container = Container(_windows, _windows_end);
        for (window* e : container)
        {
            if (e->type == type)
            {
                return e;
            }
        }

        return nullptr;
    }

    // 0x004C9B56
    window* find(window_type type, window_number number)
    {
        auto container = Container(_windows, _windows_end);
        for (window* e : container)
        {
            if (e->type == type && e->number == number)
            {
                return e;
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
        while (w >= _windows)
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
                return find_at_alt(x, y);
            }

            return w;
        }

        return nullptr;
    }

    // 0x004CB966
    void invalidate(window_type type)
    {
        auto container = Container(_windows, _windows_end);
        for (window* w : container)
        {
            if (w->type != type)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidate(window_type type, window_number number)
    {
        auto container = Container(_windows, _windows_end);
        for (window* w : container)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidate_widget(window_type type, window_number number, uint8_t widget_index)
    {
        auto container = Container(_windows, _windows_end);
        for (window* w : container)
        {
            auto widget = &w->widgets[widget_index];

            if (widget->left != -2)
            {
                gfx::set_dirty_blocks(
                    w->x + widget->left,
                    w->y + widget->top,
                    w->x + widget->right + 1,
                    w->y + widget->bottom + 1);
            }
        }
    }

    // 0x004CC692
    void close(window_type type)
    {
        bool loop = true;
        while (loop)
        {
            loop = false;
            auto container = Container(_windows, _windows_end);
            for (window* w : container)
            {
                if (w->type != type)
                    continue;

                close(w);
                loop = true;
                break;
            }
        }
    }

    // 0x004CC692
    void close(window_type type, uint16_t id)
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
        void* events)
    {
        registers regs;
        regs.eax = (y << 16) | (x & 0xFFFF);
        regs.ebx = (height << 16) | (width & 0xFFFF);
        regs.ecx = (uint8_t)type | (flags << 8);
        regs.edx = (int32_t)events;
        call(0x004C9F5D, regs);
        return (window*)regs.esi;
    }

    window* create_window_centred(window_type type, int32_t width, int32_t height, int32_t flags, void* events)
    {
        auto x = (ui::width() / 2) - (width / 2);
        auto y = std::max(28, (ui::height() / 2) - (height / 2));
        return create_window(type, x, y, width, height, flags, events);
    }

    void init_scroll_widgets(window* window)
    {
        registers regs;
        regs.esi = (uint32_t)window;
        call(0x4ca17f, regs);
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

    // 0x004CD3D0
    void dispatch_update_all()
    {
        _523508++;
        companymgr::updating_company_id(companymgr::get_controlling_id());

        for (ui::window* w = _windows_end - 1; w >= _windows; w--)
        {
            w->call_update();
        }

        call(0x4CE6FF);
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

    void sub_45F18B()
    {
        window* w = _windows_end;
        while (w >= _windows)
        {
            w->call_21();
        }
    }

    // 0x004CD296
    void relocate_windows()
    {
        int16_t newLocation = 8;
        auto container = Container(_windows, _windows_end);
        for (window* w : container)
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

        auto container = Container(_windows, _windows_end);
        for (window* w : container)
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

    void sub_4B93A5(uint16_t arg)
    {

        auto container = Container(_windows, _windows_end);
        for (window* w : container)
        {
            if (w->type != window_type::vehicle)
                continue;

            if (w->number != arg)
                continue;

            if (w->var_870 != 4)
                continue;

            w->invalidate();
        }
    }

    void close_topmost()
    {
        close(window_type::dropdown, 0);

        auto container = Container(_windows, _windows_end);
        for (window* w : container)
        {
            if (w->flags & window_flags::stick_to_back)
                continue;

            if (w->flags & window_flags::stick_to_front)
                continue;

            close(w);
            break;
        }
    }
}
