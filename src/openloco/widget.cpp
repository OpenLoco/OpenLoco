#include "widget.h"
#include "Window.h"
#include "graphics/colours.h"
#include "input.h"
#include "interop/interop.hpp"
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

    void draw_11_c(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string);
    void draw_14(gfx::GraphicsContext* context, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string);

    static void sub_4CF3EB(const gfx::GraphicsContext* context, const Window* window, const widget_t* widget, int16_t x, int16_t y, uint8_t colour, int16_t width)
    {
        registers regs;
        regs.eax = colour;
        regs.bx = width;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (uint32_t)window;
        regs.edi = (uint32_t)context;
        regs.ebp = (uint32_t)widget;
        call(0x004CF3EB, regs);
    }

    // 0x004CAB8E
    static void draw_resize_handle(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint8_t colour)
    {
        if (!(window->flags & WindowFlags::resizable))
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
        gfx::draw_image(context, x, y, image);
    }

    void sub_4CADE8(gfx::GraphicsContext* context, const Window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated)
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
                gfx::draw_image_solid(context, x + 1, y + 1, image, c);
                c = openloco::colour::get_shade(colour & 0x7F, 2);
                gfx::draw_image_solid(context, x, y, image, c);
            }
            else
            {
                c = openloco::colour::get_shade(colour & 0x7F, 6);
                gfx::draw_image_solid(context, x + 1, y + 1, image, c);
                c = openloco::colour::get_shade(colour & 0x7F, 4);
                gfx::draw_image_solid(context, x, y, image, c);
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

        gfx::draw_image(context, x, y, image);
    }

    // 0x004CAB58
    void draw_1(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        gfx::fill_rect_inset(context, window->x + widget->left, window->y + widget->top, window->x + widget->right, window->y + widget->bottom, colour, flags);

        draw_resize_handle(context, window, widget, colour);
    }

    // 0x004CAAB9
    void draw_2(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        gfx::GraphicsContext* clipped = nullptr;
        if (gfx::clipGraphicsContext(&clipped, context, widget->left + window->x, widget->top + window->y, widget->right - widget->left, 41))
        {
            uint32_t image;
            if (window->flags & WindowFlags::flag_11)
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
        if (window->flags & WindowFlags::flag_11)
        {
            shade = openloco::colour::get_shade(colour, 3);
        }
        else
        {
            shade = openloco::colour::get_shade(colour, 1);
        }

        gfx::fill_rect(
            context,
            window->x + widget->right,
            window->y + widget->top,
            window->x + widget->right,
            window->y + widget->top + 40,
            shade);

        draw_resize_handle(context, window, widget, colour);
    }

    void draw_3(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
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
            gfx::fill_rect_inset(context, left, top, right, bottom, colour, flags);
            return;
        }

        if (window->flags & WindowFlags::flag_6)
        {
            gfx::fill_rect(context, left, top, right, bottom, 0x2000000 | 52);
        }

        gfx::fill_rect_inset(context, left, top, right, bottom, colour, flags);

        if (widget->content == -1)
        {
            return;
        }

        sub_4CADE8(context, window, widget, colour, enabled, disabled, activated);
    }

    // 0x004CABFE
    void draw_5(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (widget->content == -1)
        {
            return;
        }

        if (!disabled)
        {
            widget::sub_4CADE8(context, window, widget, colour, enabled, disabled, activated);
            return;
        }

        if (widget->type == widget_type::wt_8)
        {
            return;
        }

        if (widget->type != widget_type::wt_7)
        {
            widget::sub_4CADE8(context, window, widget, colour, enabled, disabled, activated);
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

        gfx::draw_image(context, window->x + widget->left, window->y + widget->top, image);
    }

    // 0x004CACD4
    void draw_9(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (!disabled && hovered)
        {
            // TODO: Fix mixed windows
            widget::draw_3(context, window, widget, flags, colour, enabled, disabled, activated);
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
                gfx::fill_rect_inset(context, l, t, r, b, colour, flags);

                return;
            }

            gfx::fill_rect_inset(context, l, t, r, b, colour, flags);
        }

        if (widget->content == -1)
        {
            return;
        }

        widget::sub_4CADE8(context, window, widget, colour, enabled, disabled, activated);
    }

    // 0x004CAC5F
    void draw_10(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered)
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

        gfx::draw_image(context, window->x + widget->left, window->y + widget->top, image);
    }

    // 0x004CB164
    void draw_11_a(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;

        if (activated)
        {
            flags |= 0x20;
        }

        gfx::fill_rect_inset(context, left, top, right, bottom, colour, flags);
    }

    // 0x004CB1BE
    void draw_13(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
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
            draw_14(context, widget, colour, disabled, x, y, string);
        }
        else
        {
            draw_11_c(context, window, widget, colour, disabled, x, y, string);
        }
    }

    // 0x004CB21D
    void draw_11_c(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        colour &= 0x7F;
        if (disabled)
        {
            colour |= 0x40;
        }

        int16_t centreX = window->x + (widget->left + widget->right + 1) / 2 - 1;
        int16_t width = widget->right - widget->left - 2;
        gfx::draw_string_centred_clipped(*context, centreX, y, width, colour, string, _commonFormatArgs);
    }

    // 0x004CB263
    void draw_14(gfx::GraphicsContext* context, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        x = x + 1;

        colour &= 0x7F;
        if (disabled)
        {
            colour |= 0x40;
        }

        int width = widget->right - widget->left - 2;
        gfx::draw_string_494BBF(*context, x, y, width, colour, string, _commonFormatArgs);
    }

    // 0x4CB2D6
    void draw_15(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled)
    {
        if (widget->content < 0)
        {
            // FIXME: Probably wrong check, but I couldn't think of another meaning
            return;
        }

        uint8_t c = 0xFD;
        if (disabled)
        {
            c = colour | 0x40;
        }

        draw_string_494B3F(*context, window->x + widget->left + 1, window->y + widget->top, c, widget->text, _commonFormatArgs);
    }

    // 0x4CB29C
    void draw_17(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        gfx::fill_rect_inset(context, window->x + widget->left, window->y + widget->top, window->x + widget->right, window->y + widget->bottom, colour, flags | 0x60);
    }

    // 0x004CA6AE
    void draw_22_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;
        gfx::fill_rect_inset(context, left, top, right, bottom, colour, flags | 0x60);
        gfx::fill_rect(context, left + 1, top + 1, right - 1, bottom - 1, 0x2000000 | 46);

        int16_t width = widget->right - widget->left - 4 - 10;
        int16_t y = widget->top + window->y + 1;
        int16_t x = widget->left + window->x + 2 + (width / 2);

        gfx::draw_string_centred_clipped(*context, x, y, width, colour::white | 0x20, widget->text, _commonFormatArgs);
    }

    // 0x004CA750
    void draw_23_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = (uint8_t)144;
        stringmgr::format_string(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t width = widget->right - widget->left - 4 - 14;
        int16_t x = widget->left + window->x + 2 + (width / 2);

        _currentFontSpriteBase = 224;
        width = gfx::clip_string(width - 8, stringFormatBuffer);

        x -= width / 2;
        int16_t y = window->y + widget->top + 1;

        sub_4CF3EB(context, window, widget, x, y, colour, width);

        gfx::draw_string(*context, x, y, colour::black, stringFormatBuffer);
    }

    // 0x004CA7F6
    void draw_24_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = (char)13;
        stringmgr::format_string(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t x = widget->left + window->x + 2;
        int16_t width = widget->right - widget->left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = 224;
        int16_t stringWidth = gfx::clip_string(width - 8, stringFormatBuffer);
        x -= (stringWidth - 1) / 2;

        gfx::draw_string(*context, x, window->y + widget->top + 1, 0x20, stringFormatBuffer);
    }

    // 0x004CA88B
    void draw_25_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = (uint8_t)146;
        stringmgr::format_string(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t x = widget->left + window->x + 2;
        int16_t width = widget->right - widget->left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = 224;
        int16_t stringWidth = gfx::clip_string(width - 8, stringFormatBuffer);
        x -= (stringWidth - 1) / 2;

        gfx::draw_string(*context, x, window->y + widget->top + 1, 0x20, stringFormatBuffer);
    }

    static void draw_hscroll(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
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
        gfx::fill_rect_inset(context, ax, cx, ax + 9, dx, colour, f);
        // popa

        // pusha
        gfx::draw_string(*context, ax + 2, cx, colour::black, (char*)0x005045F2);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 8)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(context, bx - 9, cx, bx, dx, colour, f);
        // popa

        // pusha
        gfx::draw_string(*context, bx - 6 - 1, cx, colour::black, (char*)0x005045F5);
        // popa

        // pusha
        gfx::fill_rect(context, ax + 10, cx, bx - 10, dx, colour::get_shade(colour, 7));
        gfx::fill_rect(context, ax + 10, cx, bx - 10, dx, 0x1000000 | colour::get_shade(colour, 3));
        // popa

        // pusha
        gfx::fill_rect(context, ax + 10, cx + 2, bx - 10, cx + 2, colour::get_shade(colour, 3));
        gfx::fill_rect(context, ax + 10, cx + 3, bx - 10, cx + 3, colour::get_shade(colour, 7));
        gfx::fill_rect(context, ax + 10, cx + 7, bx - 10, cx + 7, colour::get_shade(colour, 3));
        gfx::fill_rect(context, ax + 10, cx + 8, bx - 10, cx + 8, colour::get_shade(colour, 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 2)
        {
            f = 0x20;
        }
        gfx::fill_rect_inset(context, ax - 1 + scroll_area->h_thumb_left, cx, ax - 1 + scroll_area->h_thumb_right, dx, colour, f);
        // popa
    }

    static void draw_vscroll(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
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
        gfx::fill_rect_inset(context, ax, cx, bx, cx + 9, colour, f);
        // popa

        // pusha
        gfx::draw_string(*context, ax + 1, cx - 1, colour::black, (char*)0x005045EC);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 0x80)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(context, ax, dx - 9, bx, dx, colour, f);
        // popa

        // pusha
        gfx::draw_string(*context, ax + 1, dx - 8 - 1, colour::black, (char*)0x005045EF);
        // popa

        // pusha
        gfx::fill_rect(context, ax, cx + 10, bx, dx - 10, colour::get_shade(colour, 7));
        gfx::fill_rect(context, ax, cx + 10, bx, dx - 10, 0x1000000 | colour::get_shade(colour, 3));
        // popa

        // pusha
        gfx::fill_rect(context, ax + 2, cx + 10, ax + 2, dx - 10, colour::get_shade(colour, 3));
        gfx::fill_rect(context, ax + 3, cx + 10, ax + 3, dx - 10, colour::get_shade(colour, 7));
        gfx::fill_rect(context, ax + 7, cx + 10, ax + 7, dx - 10, colour::get_shade(colour, 3));
        gfx::fill_rect(context, ax + 8, cx + 10, ax + 8, dx - 10, colour::get_shade(colour, 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & 0x20)
        {
            f = flags | 0x20;
        }
        gfx::fill_rect_inset(context, ax, cx - 1 + scroll_area->v_thumb_top, bx, cx - 1 + scroll_area->v_thumb_bottom, colour, f);
        // popa
    }

    // 0x004CB31C
    void draw_26(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index)
    {
        int16_t left = window->x + widget->left;
        int16_t top = window->y + widget->top;
        int16_t right = window->x + widget->right;
        int16_t bottom = window->y + widget->bottom;

        gfx::fill_rect_inset(context, left, top, right, bottom, colour, flags | 0x60);

        left++;
        top++;
        right--;
        bottom--;

        ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        _currentFontSpriteBase = 224;
        if (scroll_area->flags & (1 << 0))
        {
            draw_hscroll(context, window, widget, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            bottom -= 11;
        }

        if (scroll_area->flags & (1 << 4))
        {
            draw_vscroll(context, window, widget, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            right -= 11;
        }

        gfx::GraphicsContext cropped = *context;
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
    void draw_27_checkbox(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (enabled)
        {
            gfx::fill_rect_inset(
                context,
                window->x + widget->left,
                window->y + widget->top,
                window->x + widget->left + 9,
                window->y + widget->bottom - 1,
                colour,
                flags | 0x60);
        }

        if (activated)
        {
            _currentFontSpriteBase = 224;
            gfx::draw_string(*context, window->x + widget->left, window->y + widget->top, colour & 0x7F, _strCheckmark);
        }
    }

    // 0x004CB080
    void draw_27_label(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled)
    {
        if (widget->content == -1)
        {
            return;
        }

        colour &= 0x7F;

        if (disabled)
        {
            colour |= 0x40;
        }

        gfx::draw_string_494B3F(*context, window->x + widget->left + 14, window->y + widget->top, colour, widget->text, _commonFormatArgs);
    }

    // 0x004CA679
    void draw_29(gfx::GraphicsContext* context, Window* window, widget_t* widget)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;
        gfx::fill_rect(context, left, top, right, bottom, colour::get_shade(colour::black, 5));
    }
}
