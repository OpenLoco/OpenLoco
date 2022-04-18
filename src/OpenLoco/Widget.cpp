#include "Widget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "Ui/ScrollView.h"
#include "Window.h"
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui
{
    int16_t Widget::mid_x() const
    {
        return (this->left + this->right) / 2;
    }

    int16_t Widget::mid_y() const
    {
        return (this->top + this->bottom) / 2;
    }

    uint16_t Widget::width() const
    {
        return (this->right - this->left) + 1;
    }

    uint16_t Widget::height() const
    {
        return (this->bottom - this->top) + 1;
    }

    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[1], 0x112C826> _commonFormatArgs;

    static loco_global<char[2], 0x005045F8> _strCheckmark;

    void draw_11_c(Gfx::Context* context, const Window* window, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, string_id string);
    void draw_14(Gfx::Context* context, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, string_id string);

    // 0x004CF3EB
    static void drawStationNameBackground(Gfx::Context* context, const Window* window, const Widget* widget, int16_t x, int16_t y, AdvancedColour colour, int16_t width)
    {
        Gfx::drawImage(context, x - 4, y, Gfx::recolour(ImageIds::curved_border_left, colour.c()));
        Gfx::drawImage(context, x + width, y, Gfx::recolour(ImageIds::curved_border_right, colour.c()));
        Gfx::fillRect(*context, x, y, x + width - 1, y + 11, Colours::getShade(colour.c(), 5));
    }

    void Widget::draw(Gfx::Context* context, Window* window, const uint64_t pressedWidgets, const uint64_t toolWidgets, const uint64_t hoveredWidgets, uint8_t& scrollviewIndex)
    {
        if ((window->flags & WindowFlags::noBackground) == 0)
        {
            // Check if widget is outside the draw region
            if (window->x + left >= context->x + context->width && window->x + right < context->x)
            {
                if (window->y + top >= context->y + context->height && window->y + bottom < context->y)
                {
                    return;
                }
            }
        }

        uint16_t widgetFlags = 0;
        if (windowColour == WindowColour::primary && window->flags & WindowFlags::flag_11)
        {
            widgetFlags = 0x80;
        }

        auto wndColour = window->getColour(windowColour);
        auto widgetIndex = this - &window->widgets[0];
        bool enabled = (window->enabledWidgets & (1ULL << widgetIndex)) != 0;
        bool disabled = (window->disabledWidgets & (1ULL << widgetIndex)) != 0;
        bool activated = (window->activatedWidgets & (1ULL << widgetIndex)) != 0;
        activated |= (pressedWidgets & (1ULL << widgetIndex)) != 0;
        activated |= (toolWidgets & (1ULL << widgetIndex)) != 0;
        bool hovered = (hoveredWidgets & (1ULL << widgetIndex)) != 0;

        switch (type)
        {
            case WidgetType::none:
            case WidgetType::end:
                break;

            case WidgetType::panel:
                drawPanel(context, window, widgetFlags, wndColour);
                break;

            case WidgetType::frame:
                drawFrame(context, window, widgetFlags, wndColour);
                break;

            case WidgetType::wt_3:
                draw_3(context, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::wt_4:
                assert(false); // Unused
                break;

            case WidgetType::wt_5:
            case WidgetType::wt_6:
            case WidgetType::toolbarTab:
            case WidgetType::tab:
                drawTab(context, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::buttonWithImage:
                drawButtonWithImage(context, window, widgetFlags, wndColour, enabled, disabled, activated, hovered);
                break;

            case WidgetType::buttonWithColour:
                drawButtonWithColour(context, window, widgetFlags, wndColour, enabled, disabled, activated, hovered);
                break;

            case WidgetType::button:
            case WidgetType::wt_12:
            case WidgetType::buttonTableHeader:
                if (type == WidgetType::wt_12)
                {
                    assert(false); // Unused
                }
                drawButton(context, window, widgetFlags, wndColour, enabled, disabled, activated);
                draw_13(context, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::wt_13:
                draw_13(context, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::wt_15:
                draw_15(context, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::groupbox:
                // NB: widget type 16 has been repurposed to add groupboxes; the original type 16 was unused.
                drawGroupbox(context, window);
                break;

            case WidgetType::textbox:
            case WidgetType::combobox:
            case WidgetType::viewport:
                drawTextBox(context, window, widgetFlags, wndColour);
                draw_15(context, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::wt_20:
            case WidgetType::wt_21:
                assert(false); // Unused
                break;

            case WidgetType::caption_22:
                draw_22_caption(context, window, widgetFlags, wndColour);
                break;

            case WidgetType::caption_23:
                draw_23_caption(context, window, widgetFlags, wndColour);
                break;

            case WidgetType::caption_24:
                draw_24_caption(context, window, widgetFlags, wndColour);
                break;

            case WidgetType::caption_25:
                draw_25_caption(context, window, widgetFlags, wndColour);
                break;

            case WidgetType::scrollview:
                drawScrollview(context, window, widgetFlags, wndColour, enabled, disabled, activated, hovered, scrollviewIndex);
                scrollviewIndex++;
                break;

            case WidgetType::checkbox:
                draw_27_checkbox(context, window, widgetFlags, wndColour, enabled, disabled, activated);
                draw_27_label(context, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::wt_28:
                assert(false); // Unused
                draw_27_label(context, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::wt_29:
                assert(false); // Unused
                draw_29(context, window);
                break;
        }
    }
    // 0x004CF487
    void Widget::drawViewportCentreButton(Gfx::Context* context, const Window* window, const WidgetIndex_t widgetIndex)
    {
        auto& widget = window->widgets[widgetIndex];
        if (Input::isHovering(window->type, window->number, widgetIndex))
        {
            Gfx::drawRect(*context, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), 0x2000000 | 54);
            Gfx::drawRect(*context, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), 0x2000000 | 52);

            uint8_t flags = 0;
            if (Input::isPressed(window->type, window->number, widgetIndex))
                flags = 0x20;

            Gfx::drawRectInset(*context, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), window->getColour(WindowColour::secondary).translucent().u8(), flags);
        }

        Gfx::drawImage(context, widget.left + window->x, widget.top + window->y, Gfx::recolour(ImageIds::centre_viewport, window->getColour(WindowColour::secondary).c()));
    }

    // 0x004CAB8E
    static void draw_resize_handle(Gfx::Context* context, const Window* window, Widget* widget, AdvancedColour colour)
    {
        if (!(window->flags & WindowFlags::resizable))
        {
            return;
        }

        if (window->minHeight == window->maxHeight || window->minWidth == window->maxWidth)
        {
            return;
        }

        int16_t x = widget->right + window->x - 18;
        int16_t y = widget->bottom + window->y - 18;
        uint32_t image = Gfx::recolour(ImageIds::window_resize_handle, colour.c());
        Gfx::drawImage(context, x, y, image);
    }

    void Widget::sub_4CADE8(Gfx::Context* context, const Window* window, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        int16_t xPlaceForImage = left + window->x;
        int16_t yPlaceForImage = top + window->y;
        uint32_t imageId = image;
        if (type == WidgetType::wt_6 || type == WidgetType::toolbarTab || type == WidgetType::tab || type == WidgetType::wt_4)
        {
            if (activated)
            {
                // TODO: remove image addition
                imageId++;
            }
        }

        if (disabled)
        {
            if (imageId & (1 << 31))
            {
                return;
            }

            imageId &= 0x7FFFF;
            uint8_t c;
            if (colour.isTranslucent())
            {
                c = Colours::getShade(colour.c(), 4);
                Gfx::drawImageSolid(context, xPlaceForImage + 1, yPlaceForImage + 1, imageId, c);
                c = Colours::getShade(colour.c(), 2);
                Gfx::drawImageSolid(context, xPlaceForImage, yPlaceForImage, imageId, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                Gfx::drawImageSolid(context, xPlaceForImage + 1, yPlaceForImage + 1, imageId, c);
                c = Colours::getShade(colour.c(), 4);
                Gfx::drawImageSolid(context, xPlaceForImage, yPlaceForImage, imageId, c);
            }

            return;
        }

        if (imageId & (1 << 31))
        {
            // 0x4CAE5F
            assert(false);
        }

        if ((imageId & Widget::imageIdColourSet) == 0)
        {
            imageId |= enumValue(colour.c()) << 19;
        }
        else
        {
            imageId &= ~Widget::imageIdColourSet;
        }

        Gfx::drawImage(context, xPlaceForImage, yPlaceForImage, imageId);
    }

    // 0x004CAB58
    void Widget::drawPanel(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        Gfx::fillRectInset(*context, window->x + left, window->y + top, window->x + right, window->y + bottom, colour.u8(), flags);

        draw_resize_handle(context, window, this, colour);
    }

    // 0x004CAAB9
    void Widget::drawFrame(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        auto clipped = Gfx::clipContext(*context, Ui::Rect(left + window->x, top + window->y, right - left, 41));
        if (clipped)
        {
            uint32_t imageId = image;
            if (window->flags & WindowFlags::flag_11)
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image, colour.c());
            }
            else
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image_alt, colour.c());
            }
            Gfx::drawImage(&*clipped, 0, 0, imageId);
        }

        uint8_t shade;
        if (window->flags & WindowFlags::flag_11)
        {
            shade = Colours::getShade(colour.c(), 3);
        }
        else
        {
            shade = Colours::getShade(colour.c(), 1);
        }

        Gfx::fillRect(
            *context,
            window->x + right,
            window->y + top,
            window->x + right,
            window->y + top + 40,
            shade);

        draw_resize_handle(context, window, this, colour);
    }

    void Widget::draw_3(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        int16_t t, l, b, r;
        t = window->y + top;
        l = window->x + left;
        r = window->x + right;
        b = window->y + bottom;

        if (activated)
        {
            flags |= 0x20;
        }

        if (content == -2)
        {
            flags |= 0x10;
            Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags);
            return;
        }

        if (window->flags & WindowFlags::flag_6)
        {
            Gfx::fillRect(*context, l, t, r, b, 0x2000000 | 52);
        }

        Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags);

        if (content == -1)
        {
            return;
        }

        sub_4CADE8(context, window, colour, enabled, disabled, activated);
    }

    // 0x004CABFE
    void Widget::drawTab(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        if (content == -1)
        {
            return;
        }

        if (!disabled)
        {
            sub_4CADE8(context, window, colour, enabled, disabled, activated);
            return;
        }

        if (type == WidgetType::tab)
        {
            return;
        }

        if (type != WidgetType::toolbarTab)
        {
            sub_4CADE8(context, window, colour, enabled, disabled, activated);
            return;
        }

        // TODO: Remove addedImage addition
        uint32_t addedImage = image + 2;

        if ((addedImage & Widget::imageIdColourSet) == 0)
        {
            addedImage |= enumValue(colour.c()) << 19;
        }
        else
        {
            addedImage &= ~Widget::imageIdColourSet;
        }

        Gfx::drawImage(context, window->x + left, window->y + top, addedImage);
    }

    // 0x004CACD4
    void Widget::drawButtonWithImage(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (!disabled && hovered)
        {
            // TODO: Fix mixed windows
            draw_3(context, window, flags, colour, enabled, disabled, activated);
            return;
        }

        int l = left + window->x;
        int t = top + window->y;
        int r = right + window->x;
        int b = bottom + window->y;

        if (activated)
        {
            flags |= 0x20;
            if (content == -2)
            {
                // 0x004CABE8

                flags |= 0x10;
                Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags);

                return;
            }

            Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags);
        }

        if (content == -1)
        {
            return;
        }

        sub_4CADE8(context, window, colour, enabled, disabled, activated);
    }

    // 0x004CAC5F
    void Widget::drawButtonWithColour(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (content == -1)
        {
            return;
        }

        uint32_t addedImage = image;

        if (enabled)
        {
            // TODO: Remove addedImage addition
            addedImage += 2;

            if (!activated)
            {
                addedImage -= 1;

                if (!hovered)
                {
                    addedImage -= 1;
                }
            }
        }

        if ((addedImage & Widget::imageIdColourSet) == 0)
        {
            addedImage |= enumValue(colour.c()) << 19;
        }
        else
        {
            addedImage &= ~Widget::imageIdColourSet;
        }

        Gfx::drawImage(context, window->x + left, window->y + top, addedImage);
    }

    // 0x004CB164
    void Widget::drawButton(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        int l = window->x + left;
        int r = window->x + right;
        int t = window->y + top;
        int b = window->y + bottom;

        if (activated)
        {
            flags |= 0x20;
        }

        Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags);
    }

    // 0x004CB1BE
    void Widget::draw_13(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        if (content == -1)
        {
            return;
        }

        int16_t x = window->x + left;
        int16_t y = window->y + std::max<int16_t>(top, (top + bottom) / 2 - 5);
        string_id string = text;

        // TODO: Refactor out Widget type check
        if (type == WidgetType::wt_12)
        {
            if (activated)
            {
                // TODO: Remove string addition
                string++;
            }
        }

        if (type == WidgetType::buttonTableHeader)
        {
            draw_14(context, this, colour, disabled, x, y, string);
        }
        else
        {
            draw_11_c(context, window, this, colour, disabled, x, y, string);
        }
    }

    // 0x004CB21D
    void draw_11_c(Gfx::Context* context, const Window* window, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        colour = colour.opaque();
        if (disabled)
        {
            colour = colour.inset();
        }

        int16_t centreX = window->x + (widget->left + widget->right + 1) / 2 - 1;
        int16_t width = widget->right - widget->left - 2;
        Gfx::drawStringCentredClipped(*context, centreX, y, width, colour, string, _commonFormatArgs);
    }

    // 0x004CB263
    void draw_14(Gfx::Context* context, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, string_id string)
    {
        x = x + 1;

        colour = colour.opaque();
        if (disabled)
        {
            colour = colour.inset();
        }

        int width = widget->right - widget->left - 2;
        Gfx::drawString_494BBF(*context, x, y, width, colour, string, _commonFormatArgs);
    }

    // 0x4CB2D6
    void Widget::draw_15(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool disabled)
    {
        if (content == -1 || content == -2)
        {
            return;
        }

        if (disabled)
        {
            colour = colour.inset();
        }
        else
        {
            colour = colour.FD();
        }

        drawString_494B3F(*context, window->x + left + 1, window->y + top, colour, text, _commonFormatArgs);
    }

    // 0x4CB29C
    void Widget::drawTextBox(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        Gfx::fillRectInset(*context, window->x + left, window->y + top, window->x + right, window->y + bottom, colour.u8(), flags | 0x60);
    }

    // 0x004CA6AE
    void Widget::draw_22_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        int l = window->x + left;
        int r = window->x + right;
        int t = window->y + top;
        int b = window->y + bottom;
        Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags | 0x60);
        Gfx::fillRect(*context, l + 1, t + 1, r - 1, b - 1, 0x2000000 | 46);

        int16_t width = r - l - 4 - 10;
        int16_t y = t + 1;
        int16_t x = l + 2 + (width / 2);

        Gfx::drawStringCentredClipped(*context, x, y, width, AdvancedColour(Colour::white).outline(), text, _commonFormatArgs);
    }

    // 0x004CA750
    void Widget::draw_23_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::colour_black;
        StringManager::formatString(&stringBuffer[1], text, _commonFormatArgs);

        int16_t width = right - left - 4 - 14;
        int16_t x = left + window->x + 2 + (width / 2);

        _currentFontSpriteBase = Font::medium_bold;
        width = Gfx::clipString(width - 8, stringBuffer);

        x -= width / 2;
        int16_t y = window->y + top + 1;

        drawStationNameBackground(context, window, this, x, y, colour, width);

        Gfx::drawString(*context, x, y, Colour::black, stringBuffer);
    }

    // 0x004CA7F6
    void Widget::draw_24_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::window_colour_1;
        StringManager::formatString(&stringBuffer[1], text, _commonFormatArgs);

        int16_t x = left + window->x + 2;
        int16_t width = right - left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = Font::medium_bold;
        int16_t stringWidth = Gfx::clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        Gfx::drawString(*context, x, window->y + top + 1, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    // 0x004CA88B
    void Widget::draw_25_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::colour_white;
        StringManager::formatString(&stringBuffer[1], text, _commonFormatArgs);

        int16_t x = left + window->x + 2;
        int16_t width = right - left - 4 - 14;
        x = x + (width / 2);

        _currentFontSpriteBase = Font::medium_bold;
        int16_t stringWidth = Gfx::clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        Gfx::drawString(*context, x, window->y + top + 1, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    static void draw_hscroll(Gfx::Context* context, const Window* window, Widget* widget, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
    {
        const auto* scroll_area = &window->scrollAreas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        cx = dx - 10;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::vscrollbarVisible)
        {
            bx -= 11;
        }

        uint16_t f;

        // pusha
        f = 0;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::hscrollbarLeftPressed)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(*context, ax, cx, ax + 9, dx, colour.u8(), f);
        // popa

        // pusha
        Gfx::drawString(*context, ax + 2, cx, Colour::black, (char*)0x005045F2);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::hscrollbarRightPressed)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(*context, bx - 9, cx, bx, dx, colour.u8(), f);
        // popa

        // pusha
        Gfx::drawString(*context, bx - 6 - 1, cx, Colour::black, (char*)0x005045F5);
        // popa

        // pusha
        Gfx::fillRect(*context, ax + 10, cx, bx - 10, dx, Colours::getShade(colour.c(), 7));
        Gfx::fillRect(*context, ax + 10, cx, bx - 10, dx, 0x1000000 | Colours::getShade(colour.c(), 3));
        // popa

        // pusha
        Gfx::fillRect(*context, ax + 10, cx + 2, bx - 10, cx + 2, Colours::getShade(colour.c(), 3));
        Gfx::fillRect(*context, ax + 10, cx + 3, bx - 10, cx + 3, Colours::getShade(colour.c(), 7));
        Gfx::fillRect(*context, ax + 10, cx + 7, bx - 10, cx + 7, Colours::getShade(colour.c(), 3));
        Gfx::fillRect(*context, ax + 10, cx + 8, bx - 10, cx + 8, Colours::getShade(colour.c(), 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::hscrollbarThumbPressed)
        {
            f = 0x20;
        }
        Gfx::fillRectInset(*context, ax - 1 + scroll_area->hThumbLeft, cx, ax - 1 + scroll_area->hThumbRight, dx, colour.u8(), f);
        // popa
    }

    static void draw_vscroll(Gfx::Context* context, const Window* window, Widget* widget, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered, int16_t scrollview_index)
    {
        const auto* scroll_area = &window->scrollAreas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        ax = bx - 10;
        if (scroll_area->flags & ScrollView::ScrollFlags::hscrollbarVisible)
        {
            dx -= 11;
        }

        uint16_t f;

        // pusha
        f = 0;
        if (scroll_area->flags & ScrollView::ScrollFlags::vscrollbarUpPressed)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(*context, ax, cx, bx, cx + 9, colour.u8(), f);
        // popa

        // pusha
        Gfx::drawString(*context, ax + 1, cx - 1, Colour::black, (char*)0x005045EC);
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & ScrollView::ScrollFlags::vscrollbarDownPressed)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(*context, ax, dx - 9, bx, dx, colour.u8(), f);
        // popa

        // pusha
        Gfx::drawString(*context, ax + 1, dx - 8 - 1, Colour::black, (char*)0x005045EF);
        // popa

        // pusha
        Gfx::fillRect(*context, ax, cx + 10, bx, dx - 10, Colours::getShade(colour.c(), 7));
        Gfx::fillRect(*context, ax, cx + 10, bx, dx - 10, 0x1000000 | Colours::getShade(colour.c(), 3));
        // popa

        // pusha
        Gfx::fillRect(*context, ax + 2, cx + 10, ax + 2, dx - 10, Colours::getShade(colour.c(), 3));
        Gfx::fillRect(*context, ax + 3, cx + 10, ax + 3, dx - 10, Colours::getShade(colour.c(), 7));
        Gfx::fillRect(*context, ax + 7, cx + 10, ax + 7, dx - 10, Colours::getShade(colour.c(), 3));
        Gfx::fillRect(*context, ax + 8, cx + 10, ax + 8, dx - 10, Colours::getShade(colour.c(), 7));
        // popa

        // pusha
        f = 0;
        if (scroll_area->flags & ScrollView::ScrollFlags::vscrollbarThumbPressed)
        {
            f = flags | 0x20;
        }
        Gfx::fillRectInset(*context, ax, cx - 1 + scroll_area->vThumbTop, bx, cx - 1 + scroll_area->vThumbBottom, colour.u8(), f);
        // popa
    }

    // 0x004CB31C
    void Widget::drawScrollview(Gfx::Context* context, Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index)
    {
        int16_t l = window->x + left;
        int16_t t = window->y + top;
        int16_t r = window->x + right;
        int16_t b = window->y + bottom;

        Gfx::fillRectInset(*context, l, t, r, b, colour.u8(), flags | 0x60);

        l++;
        t++;
        r--;
        b--;

        const auto* scroll_area = &window->scrollAreas[scrollview_index];

        _currentFontSpriteBase = Font::medium_bold;
        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::hscrollbarVisible)
        {
            draw_hscroll(context, window, this, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            b -= 11;
        }

        if (scroll_area->flags & Ui::ScrollView::ScrollFlags::vscrollbarVisible)
        {
            draw_vscroll(context, window, this, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            r -= 11;
        }

        Gfx::Context cropped = *context;
        b++;
        r++;

        if (l > cropped.x)
        {
            int offset = l - cropped.x;
            cropped.width -= offset;
            cropped.x = l;
            cropped.pitch += offset;

            cropped.bits += offset;
        }

        int16_t bp = cropped.x + cropped.width - r;
        if (bp > 0)
        {
            cropped.width -= bp;
            cropped.pitch += bp;
        }

        if (t > cropped.y)
        {
            int offset = t - cropped.y;
            cropped.height -= offset;
            cropped.y = t;

            int aex = (cropped.pitch + cropped.width) * offset;
            cropped.bits += aex;
        }

        bp = cropped.y + cropped.height - b;
        if (bp > 0)
        {
            cropped.height -= bp;
        }

        if (cropped.width > 0 && cropped.height > 0)
        {
            cropped.x -= l - scroll_area->contentOffsetX;
            cropped.y -= t - scroll_area->contentOffsetY;

            window->callDrawScroll(&cropped, scrollview_index);
        }
    }

    // 0x004CB00B
    void Widget::draw_27_checkbox(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        if (enabled)
        {
            Gfx::fillRectInset(
                *context,
                window->x + left,
                window->y + top,
                window->x + left + 9,
                window->y + bottom - 1,
                colour.u8(),
                flags | 0x60);
        }

        if (activated)
        {
            _currentFontSpriteBase = Font::medium_bold;
            Gfx::drawString(*context, window->x + left, window->y + top, colour.opaque(), _strCheckmark);
        }
    }

    // 0x004CB080
    void Widget::draw_27_label(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool disabled)
    {
        if (content == -1)
        {
            return;
        }

        colour = colour.opaque();

        if (disabled)
        {
            colour = colour.inset();
        }

        Gfx::drawString_494B3F(*context, window->x + left + 14, window->y + top, colour, text, _commonFormatArgs);
    }

    // 0x004CA679
    void Widget::draw_29(Gfx::Context* context, const Window* window)
    {
        int l = window->x + left;
        int r = window->x + right;
        int t = window->y + top;
        int b = window->y + bottom;
        Gfx::fillRect(*context, l, t, r, b, Colours::getShade(Colour::black, 5));
    }

    void Widget::drawGroupbox(Gfx::Context* const context, const Window* window)
    {
        const auto colour = window->getColour(windowColour).opaque();
        int32_t l = window->x + left + 5;
        int32_t t = window->y + top;
        int32_t r = window->x + right;
        int32_t b = window->y + bottom;
        int32_t textEndPos = l;

        // First, draw the label text, if any.
        if (text != StringIds::null)
        {
            char buffer[512] = { 0 };
            StringManager::formatString(buffer, sizeof(buffer), text);

            Gfx::drawString(*context, l, t, colour, buffer);
            textEndPos = l + Gfx::getStringWidth(buffer) + 1;
        }

        // Prepare border dimensions
        l = window->x + left;
        t = window->y + top + 4;
        r = window->x + right;
        b = window->y + bottom;

        // Border left of text
        Gfx::fillRect(*context, l, t, l + 4, t, Colours::getShade(colour.c(), 4));
        Gfx::fillRect(*context, l + 1, t + 1, l + 4, t + 1, Colours::getShade(colour.c(), 7));

        // Border right of text
        Gfx::fillRect(*context, textEndPos, t, r - 1, t, Colours::getShade(colour.c(), 4));
        Gfx::fillRect(*context, textEndPos, t + 1, r - 2, t + 1, Colours::getShade(colour.c(), 7));

        // Border right
        Gfx::fillRect(*context, r - 1, t + 1, r - 1, b - 1, Colours::getShade(colour.c(), 4));
        Gfx::fillRect(*context, r, t, r, b, Colours::getShade(colour.c(), 7));

        // Border bottom
        Gfx::fillRect(*context, l, b - 1, r - 2, b - 1, Colours::getShade(colour.c(), 4));
        Gfx::fillRect(*context, l, b, r - 1, b, Colours::getShade(colour.c(), 7));

        // Border left
        Gfx::fillRect(*context, l, t + 1, l, b - 2, Colours::getShade(colour.c(), 4));
        Gfx::fillRect(*context, l + 1, t + 2, l + 1, b - 2, Colours::getShade(colour.c(), 7));
    }

    // 0x004CF194
    void Widget::drawTab(Window* w, Gfx::Context* ctx, int32_t imageId, WidgetIndex_t index)
    {
        auto widget = &w->widgets[index];

        Ui::Point pos = {};
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
        else if (Input::state() == Input::State::widgetPressed)
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

            Gfx::drawImage(ctx, pos.x, pos.y, Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33));
            Gfx::drawRect(*ctx, pos.x, pos.y + 26, 31, 1, Colours::getShade(w->getColour(WindowColour::secondary).c(), 7));
        }
    }
}
