#include "windowmgr.h"
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

    loco_global<uint8_t, 0x005233B6> _current_modal_type;
    loco_global<window[12], 0x011370AC> _windows;
    loco_global<window*, 0x0113D754> _windows_end;

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
        registers regs;
        call(0x004CE438, regs);
        return (window*)regs.esi;
    }

    // 0x004C9B56
    window* find(window_type type)
    {
        registers regs;
        regs.cx = (uint8_t)type | find_flag::by_type;
        call(0x004C9B56, regs);
        return (window*)regs.esi;
    }

    // 0x004C9B56
    window* find(window_type type, uint16_t id)
    {
        registers regs;
        regs.cl = (uint8_t)type;
        regs.dx = id;
        call(0x004C9B56, regs);
        return (window*)regs.esi;
    }

    // 0x004C9A95
    window* find_at(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        call(0x004C9A95, regs);
        return (window*)regs.esi;
    }

    // 0x004CB966
    void invalidate(window_type type)
    {
        auto w = find(type);
        if (w != nullptr)
        {
            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidate(window_type type, uint16_t id)
    {
        auto w = find(type, id);
        if (w != nullptr)
        {
            w->invalidate();
        }
    }

    // 0x004CC692
    void close(window_type type)
    {
        registers regs;
        regs.cx = (uint8_t)type | find_flag::by_type;
        call(0x004CC692, regs);
    }

    // 0x004CC692
    void close(window_type type, uint16_t id)
    {
        registers regs;
        regs.cl = (uint8_t)type;
        regs.dx = id;
        call(0x004CC692, regs);
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
        auto dpi = gfx::drawpixelinfo_t();
        memcpy(&dpi, _dpi, sizeof(gfx::drawpixelinfo_t));

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
}
