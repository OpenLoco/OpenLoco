#include "Widget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "Ptr.h"
#include "Ui/ScrollView.h"
#include "Window.h"
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui
{
    int16_t widget_t::mid_x() const
    {
        return (this->left + this->right) / 2;
    }

    int16_t widget_t::mid_y() const
    {
        return (this->top + this->bottom) / 2;
    }

    uint16_t widget_t::width() const
    {
        return (this->right - this->left) + 1;
    }

    uint16_t widget_t::height() const
    {
        return (this->bottom - this->top) + 1;
    }
}

namespace OpenLoco::Ui::Widget
{

    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> stringFormatBuffer;
    static loco_global<char[1], 0x112C826> _commonFormatArgs;

    static loco_global<char[2], 0x005045F8> _strCheckmark;

    void draw_11_c(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string);
    void draw_14(Gfx::drawpixelinfo_t* dpi, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string);

    static void sub_4CF3EB(const Gfx::drawpixelinfo_t* dpi, const window* window, const widget_t* widget, int16_t x, int16_t y, uint8_t colour, int16_t width)
    {
        registers regs;
        regs.eax = colour;
        regs.bx = width;
        regs.cx = x;
        regs.dx = y;
        regs.esi = ToInt(window);
        regs.edi = ToInt(dpi);
        regs.ebp = ToInt(widget);
        call(0x004CF3EB, regs);
    }

    // 0x004CF487
    void drawViewportCentreButton(Gfx::drawpixelinfo_t* dpi, const window* window, const widget_index widgetIndex)
    {
        auto& widget = window->widgets[widgetIndex];
        if (Input::isHovering(window->type, window->number, widgetIndex))
        {
            Gfx::drawRect(dpi, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), 0x2000000 | 54);
            Gfx::drawRect(dpi, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), 0x2000000 | 52);

            uint8_t flags = 0;
            if (Input::isPressed(window->type, window->number, widgetIndex))
                flags = 0x20;

            Gfx::drawRectInset(dpi, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), Colour::translucent(window->colours[1]), flags);
        }

        Gfx::drawImage(dpi, widget.left + window->x, widget.top + window->y, Gfx::recolour(ImageIds::centre_viewport, window->colours[1]));
    }

    // 0x004CAB8E
    static void draw_resize_handle(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint8_t colour)
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
        Gfx::drawImage(dpi, x, y, image);
    }

    void sub_4CADE8(Gfx::drawpixelinfo_t* dpi, const window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated)
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
            if (colour & OpenLoco::Colour::translucent_flag)
            {
                c = OpenLoco::Colour::getShade(colour & 0x7F, 4);
                Gfx::drawImageSolid(dpi, x + 1, y + 1, image, c);
                c = OpenLoco::Colour::getShade(colour & 0x7F, 2);
                Gfx::drawImageSolid(dpi, x, y, image, c);
            }
            else
            {
                c = OpenLoco::Colour::getShade(colour & 0x7F, 6);
                Gfx::drawImageSolid(dpi, x + 1, y + 1, image, c);
                c = OpenLoco::Colour::getShade(colour & 0x7F, 4);
                Gfx::drawImageSolid(dpi, x, y, image, c);
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

        Gfx::drawImage(dpi, x, y, image);
    }

    // 0x004CAB58
    void drawPanel(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        Gfx::fillRectInset(dpi, window->x + widget->left, window->y + widget->top, window->x + widget->right, window->y + widget->bottom, colour, flags);

        draw_resize_handle(dpi, window, widget, colour);
    }

    // 0x004CAAB9
    void drawFrame(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        Gfx::drawpixelinfo_t* clipped = nullptr;
        if (Gfx::clipDrawpixelinfo(&clipped, dpi, widget->left + window->x, widget->top + window->y, widget->right - widget->left, 41))
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
            Gfx::drawImage(clipped, 0, 0, image);
        }

        uint8_t shade;
        if (window->flags & WindowFlags::flag_11)
        {
            shade = OpenLoco::Colour::getShade(colour, 3);
        }
        else
        {
            shade = OpenLoco::Colour::getShade(colour, 1);
        }

        Gfx::fillRect(
            dpi,
            window->x + widget->right,
            window->y + widget->top,
            window->x + widget->right,
            window->y + widget->top + 40,
            shade);

        draw_resize_handle(dpi, window, widget, colour);
    }

    void draw_3(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
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
            Gfx::fillRectInset(dpi, left, top, right, bottom, colour, flags);
            return;
        }

        if (window->flags & WindowFlags::flag_6)
        {
            Gfx::fillRect(dpi, left, top, right, bottom, 0x2000000 | 52);
        }

        Gfx::fillRectInset(dpi, left, top, right, bottom, colour, flags);

        if (widget->content == -1)
        {
            return;
        }

        sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
    }

    // 0x004CABFE
    void draw_5(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (widget->content == -1)
        {
            return;
        }

        if (!disabled)
        {
            Widget::sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
            return;
        }

        if (widget->type == widget_type::wt_8)
        {
            return;
        }

        if (widget->type != widget_type::wt_7)
        {
            Widget::sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
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

        Gfx::drawImage(dpi, window->x + widget->left, window->y + widget->top, image);
    }

    // 0x004CACD4
    void draw_9(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (!disabled && hovered)
        {
            // TODO: Fix mixed windows
            Widget::draw_3(dpi, window, widget, flags, colour, enabled, disabled, activated);
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
                Gfx::fillRectInset(dpi, l, t, r, b, colour, flags);

                return;
            }

            Gfx::fillRectInset(dpi, l, t, r, b, colour, flags);
        }

        if (widget->content == -1)
        {
            return;
        }

        Widget::sub_4CADE8(dpi, window, widget, colour, enabled, disabled, activated);
    }

    // 0x004CAC5F
    void draw_10(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered)
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

        Gfx::drawImage(dpi, window->x + widget->left, window->y + widget->top, image);
    }

    // 0x004CB164
    void draw_11_a(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;

        if (activated)
        {
            flags |= 0x20;
        }

        Gfx::fillRectInset(dpi, left, top, right, bottom, colour, flags);
    }

    // 0x004CB1BE
    void draw_13(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
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
    void draw_11_c(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        colour &= 0x7F;
        if (disabled)
        {
            colour |= FormatFlags::textflag_6;
        }

        int16_t centreX = window->x + (widget->left + widget->right + 1) / 2 - 1;
        int16_t width = widget->right - widget->left - 2;
        Gfx::drawStringCentredClipped(*dpi, centreX, y, width, colour, string, _commonFormatArgs);
    }

    // 0x004CB263
    void draw_14(Gfx::drawpixelinfo_t* dpi, widget_t* widget, uint8_t colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        x = x + 1;

        colour &= 0x7F;
        if (disabled)
        {
            colour |= FormatFlags::textflag_6;
        }

        int width = widget->right - widget->left - 2;
        Gfx::drawString_494BBF(*dpi, x, y, width, colour, string, _commonFormatArgs);
    }

    // 0x4CB2D6
    void draw_15(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled)
    {
        if (widget->content == -1 || widget->content == -2)
        {
            return;
        }

        uint8_t c = FormatFlags::fd;
        if (disabled)
        {
            c = colour | FormatFlags::textflag_6;
        }

        drawString_494B3F(*dpi, window->x + widget->left + 1, window->y + widget->top, c, widget->text, _commonFormatArgs);
    }

    // 0x4CB29C
    void draw_17(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        Gfx::fillRectInset(dpi, window->x + widget->left, window->y + widget->top, window->x + widget->right, window->y + widget->bottom, colour, flags | 0x60);
    }

    // 0x004CA6AE
    void draw_22_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;
        Gfx::fillRectInset(dpi, left, top, right, bottom, colour, flags | 0x60);
        Gfx::fillRect(dpi, left + 1, top + 1, right - 1, bottom - 1, 0x2000000 | 46);

        int16_t width = widget->right - widget->left - 4 - 10;
        int16_t y = widget->top + window->y + 1;
        int16_t x = widget->left + window->x + 2 + (width / 2);

        Gfx::drawStringCentredClipped(*dpi, x, y, width, Colour::white | FormatFlags::textflag_5, widget->text, _commonFormatArgs);
    }

    // 0x004CA750
    void draw_23_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = ControlCodes::colour_black;
        StringManager::formatString(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t width = widget->right - widget->left - 4 - 14;
        int16_t x = widget->left + window->x + 2 + (width / 2);

        _currentFontSpriteBase = Font::medium_bold;
        width = Gfx::clipString(width - 8, stringFormatBuffer);

        x -= width / 2;
        int16_t y = window->y + widget->top + 1;

        sub_4CF3EB(dpi, window, widget, x, y, colour, width);

        Gfx::drawString(dpi, x, y, Colour::black, stringFormatBuffer);
    }

    // 0x004CA7F6
    void draw_24_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = ControlCodes::window_colour_1;
        StringManager::formatString(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t x = widget->left + window->x + 2;
        int16_t width = widget->right - widget->left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = Font::medium_bold;
        int16_t stringWidth = Gfx::clipString(width - 8, stringFormatBuffer);
        x -= (stringWidth - 1) / 2;

        Gfx::drawString(dpi, x, window->y + widget->top + 1, FormatFlags::textflag_5 | Colour::black, stringFormatBuffer);
    }

    // 0x004CA88B
    void draw_25_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour)
    {
        stringFormatBuffer[0] = ControlCodes::colour_white;
        StringManager::formatString(&stringFormatBuffer[1], widget->text, _commonFormatArgs);

        int16_t x = widget->left + window->x + 2;
        int16_t width = widget->right - widget->left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = Font::medium_bold;
        int16_t stringWidth = Gfx::clipString(width - 8, stringFormatBuffer);
        x -= (stringWidth - 1) / 2;

        Gfx::drawString(dpi, x, window->y + widget->top + 1, FormatFlags::textflag_5 | Colour::black, stringFormatBuffer);
    }

    static void draw_hscroll(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
    {
        Ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        cx = dx - 10;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::VSCROLLBAR_VISIBLE)
        {
            bx -= 11;
        }

        uint16_t f;

        // pusha
        f = 0;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::HSCROLLBAR_LEFT_PRESSED)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(dpi, ax, cx, ax + 9, dx, colour, f);
        // popa

        // pusha
        Gfx::drawString(dpi, ax + 2, cx, Colour::black, (char*)0x005045F2);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::HSCROLLBAR_RIGHT_PRESSED)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(dpi, bx - 9, cx, bx, dx, colour, f);
        // popa

        // pusha
        Gfx::drawString(dpi, bx - 6 - 1, cx, Colour::black, (char*)0x005045F5);
        // popa

        // pusha
        Gfx::fillRect(dpi, ax + 10, cx, bx - 10, dx, Colour::getShade(colour, 7));
        Gfx::fillRect(dpi, ax + 10, cx, bx - 10, dx, 0x1000000 | Colour::getShade(colour, 3));
        // popa

        // pusha
        Gfx::fillRect(dpi, ax + 10, cx + 2, bx - 10, cx + 2, Colour::getShade(colour, 3));
        Gfx::fillRect(dpi, ax + 10, cx + 3, bx - 10, cx + 3, Colour::getShade(colour, 7));
        Gfx::fillRect(dpi, ax + 10, cx + 7, bx - 10, cx + 7, Colour::getShade(colour, 3));
        Gfx::fillRect(dpi, ax + 10, cx + 8, bx - 10, cx + 8, Colour::getShade(colour, 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::HSCROLLBAR_THUMB_PRESSED)
        {
            f = 0x20;
        }
        Gfx::fillRectInset(dpi, ax - 1 + scroll_area->h_thumb_left, cx, ax - 1 + scroll_area->h_thumb_right, dx, colour, f);
        // popa
    }

    static void draw_vscroll(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
    {
        Ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        ax = bx - 10;
        if (scroll_area->flags & ScrollView::ScrollFlags::HSCROLLBAR_VISIBLE)
        {
            dx -= 11;
        }

        uint16_t f;

        // pusha
        f = 0;
        if (scroll_area->flags & ScrollView::ScrollFlags::VSCROLLBAR_UP_PRESSED)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(dpi, ax, cx, bx, cx + 9, colour, f);
        // popa

        // pusha
        Gfx::drawString(dpi, ax + 1, cx - 1, Colour::black, (char*)0x005045EC);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & ScrollView::ScrollFlags::VSCROLLBAR_DOWN_PRESSED)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(dpi, ax, dx - 9, bx, dx, colour, f);
        // popa

        // pusha
        Gfx::drawString(dpi, ax + 1, dx - 8 - 1, Colour::black, (char*)0x005045EF);
        // popa

        // pusha
        Gfx::fillRect(dpi, ax, cx + 10, bx, dx - 10, Colour::getShade(colour, 7));
        Gfx::fillRect(dpi, ax, cx + 10, bx, dx - 10, 0x1000000 | Colour::getShade(colour, 3));
        // popa

        // pusha
        Gfx::fillRect(dpi, ax + 2, cx + 10, ax + 2, dx - 10, Colour::getShade(colour, 3));
        Gfx::fillRect(dpi, ax + 3, cx + 10, ax + 3, dx - 10, Colour::getShade(colour, 7));
        Gfx::fillRect(dpi, ax + 7, cx + 10, ax + 7, dx - 10, Colour::getShade(colour, 3));
        Gfx::fillRect(dpi, ax + 8, cx + 10, ax + 8, dx - 10, Colour::getShade(colour, 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & ScrollView::ScrollFlags::VSCROLLBAR_THUMB_PRESSED)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(dpi, ax, cx - 1 + scroll_area->v_thumb_top, bx, cx - 1 + scroll_area->v_thumb_bottom, colour, f);
        // popa
    }

    // 0x004CB31C
    void drawScrollview(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index)
    {
        int16_t left = window->x + widget->left;
        int16_t top = window->y + widget->top;
        int16_t right = window->x + widget->right;
        int16_t bottom = window->y + widget->bottom;

        Gfx::fillRectInset(dpi, left, top, right, bottom, colour, flags | 0x60);

        left++;
        top++;
        right--;
        bottom--;

        Ui::scroll_area_t* scroll_area = &window->scroll_areas[scrollview_index];

        _currentFontSpriteBase = Font::medium_bold;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::HSCROLLBAR_VISIBLE)
        {
            draw_hscroll(dpi, window, widget, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            bottom -= 11;
        }

        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::VSCROLLBAR_VISIBLE)
        {
            draw_vscroll(dpi, window, widget, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            right -= 11;
        }

        Gfx::drawpixelinfo_t cropped = *dpi;
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
            cropped.x -= left - scroll_area->contentOffsetX;
            cropped.y -= top - scroll_area->contentOffsetY;

            window->callDrawScroll(&cropped, scrollview_index);
        }
    }

    // 0x004CB00B
    void draw_27_checkbox(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated)
    {
        if (enabled)
        {
            Gfx::fillRectInset(
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
            _currentFontSpriteBase = Font::medium_bold;
            Gfx::drawString(dpi, window->x + widget->left, window->y + widget->top, colour & 0x7F, _strCheckmark);
        }
    }

    // 0x004CB080
    void draw_27_label(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled)
    {
        if (widget->content == -1)
        {
            return;
        }

        colour &= 0x7F;

        if (disabled)
        {
            colour |= FormatFlags::textflag_6;
        }

        Gfx::drawString_494B3F(*dpi, window->x + widget->left + 14, window->y + widget->top, colour, widget->text, _commonFormatArgs);
    }

    // 0x004CA679
    void draw_29(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget)
    {
        int left = window->x + widget->left;
        int right = window->x + widget->right;
        int top = window->y + widget->top;
        int bottom = window->y + widget->bottom;
        Gfx::fillRect(dpi, left, top, right, bottom, Colour::getShade(Colour::black, 5));
    }

    // 0x004CF194
    void draw_tab(window* w, Gfx::drawpixelinfo_t* ctx, int32_t imageId, widget_index index)
    {
        auto widget = &w->widgets[index];

        Gfx::point_t pos = {};
        pos.x = widget->left + w->x;
        pos.y = widget->top + w->y;

        if (w->isDisabled(index))
        {
            return; // 0x8000
        }

        bool isActivated = false;
        if (w->isActivated(index))
        {
            isActivated = true;
        }
        else if (Input::state() == Input::input_state::widget_pressed)
        {
            isActivated = Input::isPressed(w->type, w->number, index);
        }

        if (imageId == -1)
        {
            return;
        }

        if (isActivated)
        {
            if (imageId != -2)
            {
                Gfx::drawImage(ctx, pos.x, pos.y, imageId);
            }
        }
        else
        {
            if (imageId != -2)
            {
                Gfx::drawImage(ctx, pos.x, pos.y + 1, imageId);
            }
            Gfx::drawImage(ctx, pos.x, pos.y, (1 << 30) | (51 << 19) | ImageIds::tab);
            Gfx::drawRect(ctx, pos.x, pos.y + 26, 31, 1, Colour::getShade(w->colours[1], 7));
        }
    }
}
