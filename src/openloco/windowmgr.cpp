#include "windowmgr.h"
#include "companymgr.h"
#include "console.h"
#include "graphics/colours.h"
#include "input.h"
#include "interop/interop.hpp"
#include "map/tile.h"
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

    loco_global<uint8_t, 0x005233B6> _current_modal_type;
    loco_global<uint32_t, 0x00523508> _523508;
    loco_global<window[12], 0x011370AC> _windows;
    loco_global<window*, 0x0113D754> _windows_end;

    struct WindowList
    {
        window* begin() const { return &_windows[0]; };
        window* end() const { return _windows_end; };
    };

    void register_hooks()
    {
        register_hook(
            0x0045F18B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                call_event_viewport_rotate_on_all_windows();
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
            [](registers& regs) -> uint8_t {
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

    // 0x004B93A5
    void sub_4B93A5(window_number number)
    {
        for (window& w : WindowList())
        {
            if (w.type != window_type::vehicle)
                continue;

            if (w.number != number)
                continue;

            if (w.var_870 != 4)
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

    static void window_viewport_get_map_coords_by_cursor(ui::window *w, int16_t *map_x, int16_t *map_y, int16_t *offset_x, int16_t *offset_y)
    {
        // Get mouse position to offset against.
        int32_t mouse_x, mouse_y;
        ui::get_cursor_pos(mouse_x, mouse_y);

        // Compute map coordinate by mouse position.
        // TODO
        get_map_coordinates_from_pos(mouse_x, mouse_y, VIEWPORT_INTERACTION_MASK_NONE, map_x, map_y, nullptr, nullptr, nullptr);

        // Get viewport coordinates centring around the tile.
        int32_t base_height = map::tile_element_height(*map_x, *map_y);
        int32_t dest_x, dest_y;
        viewport* v = w->viewports[0];
        centre_2d_coordinates(*map_x, *map_y, base_height, &dest_x, &dest_y, v);

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int32_t rebased_x = ((w->width >> 1) - mouse_x) * (1 << v->zoom),
            rebased_y = ((w->height >> 1) - mouse_y) * (1 << v->zoom);

        // Compute cursor offset relative to tile.
        viewport_config* vc = &w->viewport_configurations[0];
        *offset_x = (vc->saved_view_x - (dest_x + rebased_x)) * (1 << v->zoom);
        *offset_y = (vc->saved_view_y - (dest_y + rebased_y)) * (1 << v->zoom);
    }

    static void window_viewport_centre_tile_around_cursor(ui::window *w, int16_t map_x, int16_t map_y, int16_t offset_x, int16_t offset_y)
    {
        // Get viewport coordinates centring around the tile.
        int32_t dest_x, dest_y;
        int32_t base_height = map::tile_element_height(map_x, map_y);
        viewport* v = w->viewports[0];
        centre_2d_coordinates(map_x, map_y, base_height, &dest_x, &dest_y, v);

        // Get mouse position to offset against.
        int32_t mouse_x, mouse_y;
        ui::get_cursor_pos(mouse_x, mouse_y);

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int32_t rebased_x = ((w->width >> 1) - mouse_x) * (1 << v->zoom),
            rebased_y = ((w->height >> 1) - mouse_y) * (1 << v->zoom);

        // Apply offset to the viewport.
        viewport_config* vc = &w->viewport_configurations[0];
        vc->saved_view_x = dest_x + rebased_x + (offset_x / (1 << v->zoom));
        vc->saved_view_y = dest_y + rebased_y + (offset_y / (1 << v->zoom));
    }

    static void viewport_zoom_set(ui::window* w, int8_t zoomLevel, bool toCursor)
    {
        viewport* v = w->viewports[0];
        viewport_config* vc = &w->viewport_configurations[0];

        zoomLevel = std::clamp<int8_t>(zoomLevel, 0, 3);
        if (v->zoom == zoomLevel)
            return;

        // Zooming to cursor? Remember where we're pointing at the moment.
        int16_t saved_map_x = 0;
        int16_t saved_map_y = 0;
        int16_t offset_x = 0;
        int16_t offset_y = 0;
        if (toCursor)
        {
            window_viewport_get_map_coords_by_cursor(w, &saved_map_x, &saved_map_y, &offset_x, &offset_y);
        }

        // Zoom in
        while (v->zoom > zoomLevel)
        {
            v->zoom--;
            vc->saved_view_x += v->view_width / 4;
            vc->saved_view_y += v->view_height / 4;
            v->view_width /= 2;
            v->view_height /= 2;
        }

        // Zoom out
        while (v->zoom < zoomLevel)
        {
            v->zoom++;
            vc->saved_view_x -= v->view_width / 2;
            vc->saved_view_y -= v->view_height / 2;
            v->view_width *= 2;
            v->view_height *= 2;
        }

        // Zooming to cursor? Centre around the tile we were hovering over just now.
        if (toCursor)
        {
            window_viewport_centre_tile_around_cursor(w, saved_map_x, saved_map_y, offset_x, offset_y);
        }

        w->invalidate();
    }

    // TODO: Move
    // 0x0045F015
    static void viewport_zoom_in(ui::window* window, bool toCursor)
    {
        if (window->viewports[0] == nullptr)
            return;

        viewport_zoom_set(window, window->viewports[0]->zoom + 1, toCursor);
    }

    // TODO: Move
    // 0x0045EFDB
    static void viewport_zoom_out(ui::window* window, bool toCursor)
    {
        if (window->viewports[0] == nullptr)
            return;

        viewport_zoom_set(window, window->viewports[0]->zoom - 1, toCursor);
    }

    // TODO: Move
    // 0x0045F04F
    static void viewport_rotate_right(ui::window* window)
    {
        registers regs;
        regs.esi = (uintptr_t)window;
        call(0x0045F04F, regs);
    }

    // TODO: Move
    // 0x0045F0ED
    static void viewport_rotate_left(ui::window* window)
    {
        registers regs;
        regs.esi = (uintptr_t)window;
        call(0x0045F0ED, regs);
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
                    viewport_rotate_right(main);
                }
                else if (wheel < 0)
                {
                    viewport_rotate_left(main);
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
                    viewport_zoom_in(window, true);
                }
                else if (wheel < 0)
                {
                    viewport_zoom_out(window, true);
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
}
