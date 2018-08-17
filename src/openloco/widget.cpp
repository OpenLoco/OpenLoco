#include "widget.h"
#include "graphics/colours.h"
#include "input.h"
#include "interop/interop.hpp"
#include "window.h"
#include <cassert>

using namespace openloco::interop;

namespace openloco::ui
{
    int16_t widget_t::mid_x()
    {
        return (this->left + this->right) / 2;
    }

    uint16_t widget_t::width()
    {
        return (this->right - this->left) + 1;
    }

    uint16_t widget_t::height()
    {
        return (this->bottom - this->top) + 1;
    }
}

namespace openloco::ui::widget
{

    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> stringFormatBuffer;
    static loco_global<char[1], 0x112C826> _commonFormatArgs;

    static loco_global<char[2], 0x005045F8> _strCheckmark;

    void draw_11_c(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string);
    void draw_14(gfx::drawpixelinfo_t* dpi, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string);

    static void sub_4CF3EB(const gfx::drawpixelinfo_t* dpi, const window* window, const widget_t* widget, int16_t x, int16_t y, uint8_t colour, int16_t width)
    {
        registers regs;
        regs.eax = colour;
        regs.bx = width;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (uint32_t)window;
        regs.edi = (uint32_t)dpi;
        regs.ebp = (uint32_t)widget;
        call(0x004CF3EB, regs);
    }

    // 0x004CAB8E
    static void draw_resize_handle(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint8_t colour)
    {
        if (!(window->flags & window_flags::resizable))
        {
            return;
        }

        if (window->min_height == window->max_height || window->min_width == window->max_width)
        {
            return;
        }

        int16_t x = widget->right + window->x - 18;
        int16_t y = widget->bottom + window->y - 18;
        uint32_t image = 0x20000000 | 2305 | (colour << 19);
        gfx::draw_image(dpi, x, y, image);
    }

    void sub_4CADE8(gfx::drawpixelinfo_t* dpi, const window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        int16_t x = widget->left + window->x;
        int16_t y = widget->top + window->y;
        uint32_t image = widget->image;

        if (widget->type == widget_type::wt_6 || widget->type == widget_type::wt_7 || widget->type == widget_type::wt_8 || widget->type == widget_type::wt_4)
        {
            if (activated)
            {
                // TODO: remove image addition
                image++;
            }
        }

        if (disabled)
        {
            if (image & (1 << 31))
            {
                return;
            }

            image &= 0x7FFFF;
            uint8_t c;
            if (colour & openloco::colour::translucent_flag)
            {
                c = openloco::colour::get_shade(colour & 0x7F, 4);
                gfx::draw_image_solid(dpi, x + 1, y + 1, image, c);
                c = openloco::colour::get_shade(colour & 0x7F, 2);
                gfx::draw_image_solid(dpi, x, y, image, c);
            }
            else
            {
                c = openloco::colour::get_shade(colour & 0x7F, 6);
                gfx::draw_image_solid(dpi, x + 1, y + 1, image, c);
                c = openloco::colour::get_shade(colour & 0x7F, 4);
                gfx::draw_image_solid(dpi, x, y, image, c);
            }

            return;
        }

        if (image & (1 << 31))
        {
            // 0x4CAE5F
            assert(false);
        }

        if ((image & (1 << 30)) == 0)
        {
            image |= colour << 19;
        }
        else
        {
            image &= ~(1 << 30);
        }

        gfx::draw_image(dpi, x, y, image);
    }

    // 0x004CAB58
    void draw_1(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        gfx::fill_rect_inset(dpi, window->x + widget->left, window->y + widget->top, window->x + widget->right, window->y + widget->bottom, colour, flags);

        draw_resize_handle(dpi, window, widget, colour);
    }

    // 0x004CAAB9
    void draw_2(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        gfx::drawpixelinfo_t* clipped = nullptr;
        if (gfx::clip_drawpixelinfo(&clipped, dpi, widget->left + window->x, widget->top + window->y, widget->right - widget->left, 41))
        {
            uint32_t image;
            if (window->flags & window_flags::flag_11)
            {
                image = 0x20000000 | 2322 | ((colour & 0x7F) << 19);
            }
            else
            {
                image = 0x20000000 | 2323 | ((colour & 0x7F) << 19);
            }
            gfx::draw_image(clipped, 0, 0, image);
        }

        uint8_t shade;
        if (window->flags & window_flags::flag_11)
        {
            shade = openloco::colour::get_shade(colour, 3);
        }
        else
        {
            shade = openloco::colour::get_shade(colour, 1);
        }

        gfx::fill_rect(
            dpi,
            window->x + widget->right,
            window->y + widget->top,
            window->x + widget->right,
            window->y + widget->top + 40,
            shade);

        draw_resize_handle(dpi, window, widget, colour);
    }

    void draw_3(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        int16_t top, left, bottom, right;
        top = window->y + widget->top;
        left = window->x + widget->left;
        right = window->x + widget->right;
        bottom = window->y + widget->bottom;

        if (activated)
        {
            flags |= 0x20;
        }

        if (widget->content == -2)
        {
            flags |= 0x10;
            gfx::fill_rect_inset(dpi, left, top, right, bottom, colour, flags);
            return;
        }

        if (window->flags & window_flags::flag_6)
        {
            gfx::fill_rect(dpi, left, top, right, bottom, 0x2000000 | 52);
        }

        gfx::fill_rect_inset(dpi, left, top, right, bottom, colour, flags);

        if (widget->content == -1)
        {
            return;
        }

        sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
    }

    // 0x004CABFE
    void draw_5(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (widget->content == -1)
        {
            return;
        }

        if (!disabled)
        {
            widget::sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
            return;
        }

        if (widget->type == widget_type::wt_8)
        {
            return;
        }

        if (widget->type != widget_type::wt_7)
        {
            widget::sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
            return;
        }

        // TODO: Remove image addition
        uint32_t image = widget->image + 2;

        if ((image & (1 << 30)) == 0)
        {
            image |= colour << 19;
        }
        else
        {
            image &= ~(1 << 30);
        }

        gfx::draw_image(dpi, window->x + widget->left, window->y + widget->top, image);
    }

    // 0x004CACD4
    void draw_9(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (!disabled && hovered)
        {
            // TODO: Fix mixed windows
            widget::draw_3(dpi, window, widget, flags, colour, enabled, disabled, activated);
            return;
        }

        int l = widget->left + window->x;
        int t = widget->top + window->y;
        int r = widget->right + window->x;
        int b = widget->bottom + window->y;

        if (activated)
        {
            flags |= 0x20;
            if (widget->content == -2)
            {
                // 0x004CABE8

                flags |= 0x10;
                gfx::fill_rect_inset(dpi, l, t, r, b, colour, flags);

                return;
            }

            gfx::fill_rect_inset(dpi, l, t, r, b, colour, flags);
        }

        if (widget->content == -1)
        {
            return;
        }

        widget::sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
    }

    // 0x004CAC5F
    void draw_10(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (widget->content == -1)
        {
            return;
        }

        uint32_t image = widget->image;

        if (enabled)
        {
            // TODO: Remove image addition
            image += 2;

            if (!activated)
            {
                image -= 1;

                if (!hovered)
                {
                    image -= 1;
                }
            }
        }

        if ((image & (1 << 30)) == 0)
        {
            image |= colour << 19;
        }
        else
        {
            image &= ~(1 << 30);
        }

        gfx::draw_image(dpi, window->x + widget->left, window->y + widget->top, image);
    }

    // 0x004CB164
    void draw_11_a(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;

        if (activated)
        {
            flags |= 0x20;
        }

        gfx::fill_rect_inset(dpi, left, top, right, bottom, colour, flags);
    }

    // 0x004CB1BE
    void draw_13(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (widget->content == -1)
        {
            return;
        }

        int16_t x = window->x + widget->left;
        int16_t y = window->y + std::max<int16_t>(widget->top, (widget->top + widget->bottom) / 2 - 5);
        string_id string = widget->text;

        // TODO: Refactor out widget_t type check
        if (widget->type == widget_type::wt_12)
        {
            if (activated)
            {
                // TODO: Remove string addition
                string++;
            }
        }

        if (widget->type == widget_type::wt_14)
        {
            draw_14(dpi, widget, colour, disabled, x, y, string);
        }
        else
        {
            draw_11_c(dpi, window, widget, colour, disabled, x, y, string);
        }
    }

    // 0x004CB21D
    void draw_11_c(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        colour &= 0x7F;
        if (disabled)
        {
            colour |= format_flags::textflag_6;
        }

        int16_t centreX = window->x + (widget->left + widget->right + 1) / 2 - 1;
        int16_t width = widget->right - widget->left - 2;
        gfx::draw_string_centred_clipped(*dpi, centreX, y, width, colour, string, _commonFormatArgs);
    }

    // 0x004CB263
    void draw_14(gfx::drawpixelinfo_t* dpi, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        x = x + 1;

        colour &= 0x7F;
        if (disabled)
        {
            colour |= format_flags::textflag_6;
        }

        int width = widget->right - widget->left - 2;
        gfx::draw_string_494BBF(*dpi, x, y, width, colour, string, _commonFormatArgs);
    }

    // 0x4CB2D6
    void draw_15(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled)
    {
        if (widget->content < 0)
        {
            // FIXME: Probably wrong check, but I couldn't think of another meaning
            return;
        }

        uint8_t c = format_flags::fd;
        if (disabled)
        {
            c = colour | format_flags::textflag_6;
        }

        draw_string_494B3F(*dpi, window->x + widget->left + 1, window->y + widget->top, c, widget->text, _commonFormatArgs);
    }

    // 0x4CB29C
    void draw_17(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        gfx::fill_rect_inset(dpi, window->x + widget->left, window->y + widget->top, window->x + widget->right, window->y + widget->bottom, colour, flags | 0x60);
    }

    // 0x004CA6AE
    void draw_22_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;
        gfx::fill_rect_inset(dpi, left, top, right, bottom, colour, flags | 0x60);
        gfx::fill_rect(dpi, left + 1, top + 1, right - 1, bottom - 1, 0x2000000 | 46);

        int16_t width = widget->right - widget->left - 4 - 10;
        int16_t y = widget->top + window->y + 1;
        int16_t x = widget->left + window->x + 2 + (width / 2);

        gfx::draw_string_centred_clipped(*dpi, x, y, width, colour::white | format_flags::textflag_5, widget->text, _commonFormatArgs);
    }

    // 0x004CA750
    void draw_23_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = control_codes::colour_black;
        stringmgr::format_string(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t width = widget->right - widget->left - 4 - 14;
        int16_t x = widget->left + window->x + 2 + (width / 2);

        _currentFontSpriteBase = font::medium_bold;
        width = gfx::clip_string(width - 8, stringFormatBuffer);

        x -= width / 2;
        int16_t y = window->y + widget->top + 1;

        sub_4CF3EB(dpi, window, widget, x, y, colour, width);

        gfx::draw_string(dpi, x, y, colour::black, stringFormatBuffer);
    }

    // 0x004CA7F6
    void draw_24_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = control_codes::window_colour_1;
        stringmgr::format_string(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t x = widget->left + window->x + 2;
        int16_t width = widget->right - widget->left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = font::medium_bold;
        int16_t stringWidth = gfx::clip_string(width - 8, stringFormatBuffer);
        x -= (stringWidth - 1) / 2;

        gfx::draw_string(dpi, x, window->y + widget->top + 1, format_flags::textflag_5 | colour::black, stringFormatBuffer);
    }

    // 0x004CA88B
    void draw_25_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = control_codes::colour_white;
        stringmgr::format_string(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t x = widget->left + window->x + 2;
        int16_t width = widget->right - widget->left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = font::medium_bold;
        int16_t stringWidth = gfx::clip_string(width - 8, stringFormatBuffer);
        x -= (stringWidth - 1) / 2;

        gfx::draw_string(dpi, x, window->y + widget->top + 1, format_flags::textflag_5 | colour::black, stringFormatBuffer);
    }

    static void draw_hscroll(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
    {
        ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        cx = dx - 10;
        if (scroll_area->flags & 0x10)
        {
            bx -= 11;
        }

        uint16_t f;

        // pusha
        f = 0;
        if (scroll_area->flags & 4)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(dpi, ax, cx, ax + 9, dx, colour, f);
        // popa

        // pusha
        gfx::draw_string(dpi, ax + 2, cx, colour::black, (char*)0x005045F2);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 8)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(dpi, bx - 9, cx, bx, dx, colour, f);
        // popa

        // pusha
        gfx::draw_string(dpi, bx - 6 - 1, cx, colour::black, (char*)0x005045F5);
        // popa

        // pusha
        gfx::fill_rect(dpi, ax + 10, cx, bx - 10, dx, colour::get_shade(colour, 7));
        gfx::fill_rect(dpi, ax + 10, cx, bx - 10, dx, 0x1000000 | colour::get_shade(colour, 3));
        // popa

        // pusha
        gfx::fill_rect(dpi, ax + 10, cx + 2, bx - 10, cx + 2, colour::get_shade(colour, 3));
        gfx::fill_rect(dpi, ax + 10, cx + 3, bx - 10, cx + 3, colour::get_shade(colour, 7));
        gfx::fill_rect(dpi, ax + 10, cx + 7, bx - 10, cx + 7, colour::get_shade(colour, 3));
        gfx::fill_rect(dpi, ax + 10, cx + 8, bx - 10, cx + 8, colour::get_shade(colour, 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 2)
        {
            f = 0x20;
        }
        gfx::fill_rect_inset(dpi, ax - 1 + scroll_area->h_thumb_left, cx, ax - 1 + scroll_area->h_thumb_right, dx, colour, f);
        // popa
    }

    static void draw_vscroll(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
    {
        ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        ax = bx - 10;
        if (scroll_area->flags & 0x1)
        {
            dx -= 11;
        }

        uint16_t f;

        // pusha
        f = 0;
        if (scroll_area->flags & 0x40)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(dpi, ax, cx, bx, cx + 9, colour, f);
        // popa

        // pusha
        gfx::draw_string(dpi, ax + 1, cx - 1, colour::black, (char*)0x005045EC);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 0x80)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(dpi, ax, dx - 9, bx, dx, colour, f);
        // popa

        // pusha
        gfx::draw_string(dpi, ax + 1, dx - 8 - 1, colour::black, (char*)0x005045EF);
        // popa

        // pusha
        gfx::fill_rect(dpi, ax, cx + 10, bx, dx - 10, colour::get_shade(colour, 7));
        gfx::fill_rect(dpi, ax, cx + 10, bx, dx - 10, 0x1000000 | colour::get_shade(colour, 3));
        // popa

        // pusha
        gfx::fill_rect(dpi, ax + 2, cx + 10, ax + 2, dx - 10, colour::get_shade(colour, 3));
        gfx::fill_rect(dpi, ax + 3, cx + 10, ax + 3, dx - 10, colour::get_shade(colour, 7));
        gfx::fill_rect(dpi, ax + 7, cx + 10, ax + 7, dx - 10, colour::get_shade(colour, 3));
        gfx::fill_rect(dpi, ax + 8, cx + 10, ax + 8, dx - 10, colour::get_shade(colour, 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 0x20)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(dpi, ax, cx - 1 + scroll_area->v_thumb_top, bx, cx - 1 + scroll_area->v_thumb_bottom, colour, f);
        // popa
    }

    // 0x004CB31C
    void draw_26(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index)
    {
        int16_t left = window->x + widget->left;
        int16_t top = window->y + widget->top;
        int16_t right = window->x + widget->right;
        int16_t bottom = window->y + widget->bottom;

        gfx::fill_rect_inset(dpi, left, top, right, bottom, colour, flags | 0x60);

        left++;
        top++;
        right--;
        bottom--;

        ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        _currentFontSpriteBase = font::medium_bold;
        if (scroll_area->flags & (1 << 0))
        {
            draw_hscroll(dpi, window, widget, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            bottom -= 11;
        }

        if (scroll_area->flags & (1 << 4))
        {
            draw_vscroll(dpi, window, widget, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            right -= 11;
        }

        gfx::drawpixelinfo_t cropped = *dpi;
        bottom++;
        right++;

        if (left > cropped.x)
        {
            int offset = left - cropped.x;
            cropped.width -= offset;
            cropped.x = left;
            cropped.pitch += offset;

            cropped.bits += offset;
        }

        int16_t bp = cropped.x + cropped.width - right;
        if (bp > 0)
        {
            cropped.width -= bp;
            cropped.pitch += bp;
        }

        if (top > cropped.y)
        {
            int offset = top - cropped.y;
            cropped.height -= offset;
            cropped.y = top;

            int aex = (cropped.pitch + cropped.width) * offset;
            cropped.bits += aex;
        }

        bp = cropped.y + cropped.height - bottom;
        if (bp > 0)
        {
            cropped.height -= bp;
        }

        if (cropped.width > 0 && cropped.height > 0)
        {
            cropped.x -= left - scroll_area->h_left;
            cropped.y -= top - scroll_area->v_top;

            window->call_draw_scroll(&cropped, scrollview_index);
        }
    }

    // 0x004CB00B
    void draw_27_checkbox(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (enabled)
        {
            gfx::fill_rect_inset(
                dpi,
                window->x + widget->left,
                window->y + widget->top,
                window->x + widget->left + 9,
                window->y + widget->bottom - 1,
                colour,
                flags | 0x60);
        }

        if (activated)
        {
            _currentFontSpriteBase = font::medium_bold;
            gfx::draw_string(dpi, window->x + widget->left, window->y + widget->top, colour & 0x7F, _strCheckmark);
        }
    }

    // 0x004CB080
    void draw_27_label(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled)
    {
        if (widget->content == -1)
        {
            return;
        }

        colour &= 0x7F;

        if (disabled)
        {
            colour |= format_flags::textflag_6;
        }

        gfx::draw_string_494B3F(*dpi, window->x + widget->left + 14, window->y + widget->top, colour, widget->text, _commonFormatArgs);
    }

    // 0x004CA679
    void draw_29(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;
        gfx::fill_rect(dpi, left, top, right, bottom, colour::get_shade(colour::black, 5));
    }
}
