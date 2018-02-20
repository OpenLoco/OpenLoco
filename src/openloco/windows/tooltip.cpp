#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui.h"
#include "../windowmgr.h"
#include <algorithm>
#include <cstring>

using namespace openloco::interop;

namespace openloco::ui::tooltip
{
    static loco_global<char[513], 0x0050ED4B> _str0337;

    static loco_global<int8_t, 0x0052336E> _52336E; // bool
    static loco_global<window_type, 0x00523381> _523381;
    static loco_global<int16_t, 0x00523382> _523382;
    static loco_global<int16_t, 0x00523384> _523384;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;

    static loco_global<ui::widget_t[1], 0x005234CC> _widgets;

    static loco_global<int32_t, 0x112C876> gCurrentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> byte_112CC04;

    static loco_global<char[512], 0x01136D90> _text;
    static loco_global<uint16_t, 0x01136F90> _lineBreakCount;

    static loco_global<int32_t, 0x01136F98> _1136F98;

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void on_close(ui::window* window);
    static void update(ui::window* window);

    void register_hooks()
    {
        register_hook(
            0x004C906B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                ui::tooltip::open((ui::window*)regs.esi, regs.edx, regs.ax, regs.bx);
                return 0;
            });
        register_hook(
            0x004C9216,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                ui::tooltip::update((ui::window*)regs.esi, regs.edx, regs.ax, regs.bx);
                return 0;
            });
        register_hook(
            0x004C9397,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                draw((ui::window*)regs.esi, (gfx::drawpixelinfo_t*)regs.edi);
                return 0;
            });
        register_hook(
            0x004C94F7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                on_close((ui::window*)regs.esi);
                return 0;
            });
        register_hook(
            0x004C94FF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                update((ui::window*)regs.esi);
                return 0;
            });
    }

    static void common(const window* window, int32_t widgetIndex, int16_t cursorX, int16_t cursorY)
    {
        {
            // format_string
            registers regs;
            regs.eax = window->widgets[widgetIndex].tooltip;
            regs.edi = (uint32_t)&byte_112CC04[0];
            regs.ecx = 0x112C826;
            call(0x004958C6, regs);
        }

        gCurrentFontSpriteBase = 224;
        int16_t strWidth;
        {
            // gfx_get_string_width_new_lined
            registers regs;
            regs.esi = (uint32_t)&byte_112CC04[0];
            call(0x00495715, regs);
            strWidth = regs.cx;
        }
        strWidth = std::max<int16_t>(strWidth, 196);

        gCurrentFontSpriteBase = 224;
        {
            // gfx_wrap_string
            registers regs;
            regs.esi = (uint32_t)&byte_112CC04[0];
            regs.di = strWidth + 1;
            call(0x00495301, regs);
            strWidth = regs.cx;
            _lineBreakCount = regs.di;
        }

        int width = strWidth + 3;
        int height = (_lineBreakCount + 1) * 10 + 4;
        _widgets[0].right = width;
        _widgets[0].bottom = height;

        std::memcpy(&_text[0], &byte_112CC04[0], 512);

        int x, y;

        int maxY = ui::height() - height;
        y = cursorY + 26; // Normally, we'd display the tooltip 26 lower
        if (y > maxY)
            // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
            // so we'll subtract a bit more
            y -= height + 40;
        y = std::clamp(y, 22, maxY);

        x = std::clamp(cursorX - (width / 2), 0, ui::width() - width);

        auto tooltip = windowmgr::create_window(
            window_type::tooltip,
            x,
            y,
            width,
            height,
            window_flags::flag_1 | window_flags::flag_4 | window_flags::flag_7,
            (void*)0x504774);
        tooltip->widgets = _widgets;
        _tooltipNotShownTicks = 0;
    }

    // 0x004C906B
    void open(ui::window* window, int32_t widgetIndex, int16_t cursorX, int16_t cursorY)
    {
        windowmgr::close(window_type::tooltip, 0);
        if (window == nullptr || widgetIndex == -1)
        {
            return;
        }

        window->call_prepare_draw();
        if (window->widgets[widgetIndex].tooltip == string_ids::null)
        {
            return;
        }

        _523381 = window->type;
        _523382 = window->number;
        _523384 = widgetIndex;

        auto showString = window->call_tooltip(widgetIndex);
        if (!showString)
        {
            return;
        }

        auto wnd = windowmgr::find(window_type::wt_12, 0);
        if (wnd != nullptr)
        {
            return;
        }

        _1136F98 = -1;

        common(window, widgetIndex, cursorX, cursorY);
    }

    // 0x004C9216
    void update(ui::window* window, int32_t widgetIndex, int16_t cursorX, int16_t cursorY)
    {
        windowmgr::close(window_type::tooltip, 0);

        _523381 = window->type;
        _523382 = window->number;
        _523384 = widgetIndex;

        auto showString = window->call_tooltip(widgetIndex);
        if (!showString)
        {
            return;
        }

        auto wnd = windowmgr::find(window_type::wt_12, 0);
        if (wnd != nullptr)
        {
            return;
        }

        _1136F98 = (uint32_t)&window->widgets[widgetIndex];

        common(window, widgetIndex, cursorX, cursorY);
    }

    // 0x004C9397
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        uint16_t x = window->x;
        uint16_t y = window->y;
        uint16_t width = window->width;
        uint16_t height = window->height;

        gfx::draw_rect(dpi, x + 1, y + 1, width - 2, height - 2, 0x2000000 | 45);
        gfx::draw_rect(dpi, x + 1, y + 1, width - 2, height - 2, 0x2000000 | (116 + objectmgr::get<interface_skin_object>()->colour_08));

        gfx::draw_rect(dpi, x, y + 2, 1, height - 4, 0x2000000 | 46);
        gfx::draw_rect(dpi, x + width - 1, y + 2, 1, height - 4, 0x2000000 | 46);
        gfx::draw_rect(dpi, x + 2, y + height - 1, width - 4, 1, 0x2000000 | 46);
        gfx::draw_rect(dpi, x + 2, y, width - 4, 1, 0x2000000 | 46);

        gfx::draw_rect(dpi, x + 1, y + 1, 1, 1, 0x2000000 | 46);
        gfx::draw_rect(dpi, x + width - 1 - 1, y + 1, 1, 1, 0x2000000 | 46);
        gfx::draw_rect(dpi, x + 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);
        gfx::draw_rect(dpi, x + width - 1 - 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);

        {
            // draw_string_centred_raw
            registers regs;
            regs.cx = ((width + 1) / 2) + x - 1;
            regs.dx = y + 1;
            regs.al = 0;
            regs.bp = _lineBreakCount;
            regs.edi = (uint32_t)dpi;
            regs.esi = (uint32_t)&_text[0];
            call(0x00494E33, regs);
        }
    }

    // 0x004C94F7
    static void on_close(ui::window* window)
    {
        _str0337[0] = '\0';
    }

    // 0x004C94FF
    static void update(ui::window* window)
    {
        if (_52336E == false)
        {
            _tooltipNotShownTicks = 0;
        }
    }
}
