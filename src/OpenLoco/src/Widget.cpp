#include "Widget.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Ui/ScrollView.h"
#include "Window.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui
{
    int16_t Widget::midX() const
    {
        return (this->left + this->right) / 2;
    }

    int16_t Widget::midY() const
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

    void draw_11_c(Gfx::RenderTarget* rt, const Window* window, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, StringId string);
    void draw_14(Gfx::RenderTarget* rt, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, StringId string);

    // 0x004CF3EB
    static void drawStationNameBackground(Gfx::RenderTarget* rt, [[maybe_unused]] const Window* window, [[maybe_unused]] const Widget* widget, int16_t x, int16_t y, AdvancedColour colour, int16_t width)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(rt, x - 4, y, Gfx::recolour(ImageIds::curved_border_left_medium, colour.c()));
        drawingCtx.drawImage(rt, x + width, y, Gfx::recolour(ImageIds::curved_border_right_medium, colour.c()));
        drawingCtx.fillRect(*rt, x, y, x + width - 1, y + 11, Colours::getShade(colour.c(), 5), Drawing::RectFlags::none);
    }

    void Widget::draw(Gfx::RenderTarget* rt, Window* window, const uint64_t pressedWidgets, const uint64_t toolWidgets, const uint64_t hoveredWidgets, uint8_t& scrollviewIndex)
    {
        if (!window->hasFlags(WindowFlags::noBackground))
        {
            // Check if widget is outside the draw region
            if (window->x + left >= rt->x + rt->width && window->x + right < rt->x)
            {
                if (window->y + top >= rt->y + rt->height && window->y + bottom < rt->y)
                {
                    return;
                }
            }
        }

        Drawing::RectInsetFlags widgetFlags = Drawing::RectInsetFlags::none;
        if (windowColour == WindowColour::primary && window->hasFlags(WindowFlags::flag_11))
        {
            widgetFlags = Drawing::RectInsetFlags::colourLight;
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
                drawPanel(rt, window, widgetFlags, wndColour);
                break;

            case WidgetType::frame:
                drawFrame(rt, window, widgetFlags, wndColour);
                break;

            case WidgetType::wt_3:
                draw_3(rt, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::wt_4:
                assert(false); // Unused
                break;

            case WidgetType::slider:
            case WidgetType::wt_6:
            case WidgetType::toolbarTab:
            case WidgetType::tab:
                drawTab(rt, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::buttonWithImage:
                drawButtonWithImage(rt, window, widgetFlags, wndColour, enabled, disabled, activated, hovered);
                break;

            case WidgetType::buttonWithColour:
                drawButtonWithColour(rt, window, widgetFlags, wndColour, enabled, disabled, activated, hovered);
                break;

            case WidgetType::button:
            case WidgetType::wt_12:
            case WidgetType::buttonTableHeader:
                if (type == WidgetType::wt_12)
                {
                    assert(false); // Unused
                }
                drawButton(rt, window, widgetFlags, wndColour, enabled, disabled, activated);
                draw_13(rt, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::wt_13:
                draw_13(rt, window, widgetFlags, wndColour, enabled, disabled, activated);
                break;

            case WidgetType::wt_15:
                draw_15(rt, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::groupbox:
                // NB: widget type 16 has been repurposed to add groupboxes; the original type 16 was unused.
                drawGroupbox(rt, window);
                break;

            case WidgetType::textbox:
            case WidgetType::combobox:
            case WidgetType::viewport:
                drawTextBox(rt, window, widgetFlags, wndColour);
                draw_15(rt, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::wt_20:
            case WidgetType::wt_21:
                assert(false); // Unused
                break;

            case WidgetType::caption_22:
                draw_22_caption(rt, window, widgetFlags, wndColour);
                break;

            case WidgetType::caption_23:
                draw_23_caption(rt, window, widgetFlags, wndColour);
                break;

            case WidgetType::caption_24:
                draw_24_caption(rt, window, widgetFlags, wndColour);
                break;

            case WidgetType::caption_25:
                draw_25_caption(rt, window, widgetFlags, wndColour);
                break;

            case WidgetType::scrollview:
                drawScrollview(rt, window, widgetFlags, wndColour, enabled, disabled, activated, hovered, scrollviewIndex);
                scrollviewIndex++;
                break;

            case WidgetType::checkbox:
                draw_27_checkbox(rt, window, widgetFlags, wndColour, enabled, disabled, activated);
                draw_27_label(rt, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::wt_28:
                assert(false); // Unused
                draw_27_label(rt, window, widgetFlags, wndColour, disabled);
                break;

            case WidgetType::wt_29:
                assert(false); // Unused
                draw_29(rt, window);
                break;
        }
    }
    // 0x004CF487
    void Widget::drawViewportCentreButton(Gfx::RenderTarget* rt, const Window* window, const WidgetIndex_t widgetIndex)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto& widget = window->widgets[widgetIndex];
        if (Input::isHovering(window->type, window->number, widgetIndex))
        {
            drawingCtx.drawRect(*rt, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), enumValue(ExtColour::translucentGrey2), Drawing::RectFlags::transparent);
            drawingCtx.drawRect(*rt, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);

            Drawing::RectInsetFlags flags = Drawing::RectInsetFlags::none;
            if (Input::isPressed(window->type, window->number, widgetIndex))
                flags = Drawing::RectInsetFlags::borderInset;

            drawingCtx.drawRectInset(*rt, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), window->getColour(WindowColour::secondary).translucent(), flags);
        }

        drawingCtx.drawImage(rt, widget.left + window->x, widget.top + window->y, Gfx::recolour(ImageIds::centre_viewport, window->getColour(WindowColour::secondary).c()));
    }

    // 0x004CAB8E
    static void draw_resize_handle(Gfx::RenderTarget* rt, const Window* window, Widget* widget, AdvancedColour colour)
    {
        if (!window->hasFlags(WindowFlags::resizable))
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
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(rt, x, y, image);
    }

    void Widget::sub_4CADE8(Gfx::RenderTarget* rt, const Window* window, AdvancedColour colour, [[maybe_unused]] bool enabled, bool disabled, bool activated)
    {
        Ui::Point placeForImage(left + window->x, top + window->y);
        const bool isColourSet = image & Widget::kImageIdColourSet;
        ImageId imageId = ImageId::fromUInt32(image & ~Widget::kImageIdColourSet);
        if (type == WidgetType::wt_6 || type == WidgetType::toolbarTab || type == WidgetType::tab || type == WidgetType::wt_4)
        {
            if (activated)
            {
                // TODO: remove image addition
                imageId = imageId.withIndexOffset(1);
            }
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        if (disabled)
        {
            // TODO: this is odd most likely this is another flag like Widget::kImageIdColourSet
            if (imageId.hasSecondary())
            {
                return;
            }

            // No colour applied image
            const auto pureImage = ImageId{ imageId.getIndex() };
            uint8_t c;
            if (colour.isTranslucent())
            {
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(*rt, placeForImage + Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 2);
                drawingCtx.drawImageSolid(*rt, placeForImage, pureImage, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                drawingCtx.drawImageSolid(*rt, placeForImage + Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(*rt, placeForImage, pureImage, c);
            }

            return;
        }

        if (!isColourSet && imageId.hasSecondary())
        {
            imageId = imageId.withSecondary(colour.c());
        }

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(colour.c());
        }

        drawingCtx.drawImage(*rt, placeForImage, imageId);
    }

    // 0x004CAB58
    void Widget::drawPanel(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(*rt, window->x + left, window->y + top, window->x + right, window->y + bottom, colour, flags);

        draw_resize_handle(rt, window, this, colour);
    }

    // 0x004CAAB9
    void Widget::drawFrame(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(left + window->x, top + window->y, right - left, 41));
        if (clipped)
        {
            uint32_t imageId = image;
            if (window->hasFlags(WindowFlags::flag_11))
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image, colour.c());
            }
            else
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image_alt, colour.c());
            }
            drawingCtx.drawImage(&*clipped, 0, 0, imageId);
        }

        uint8_t shade;
        if (window->hasFlags(WindowFlags::flag_11))
        {
            shade = Colours::getShade(colour.c(), 3);
        }
        else
        {
            shade = Colours::getShade(colour.c(), 1);
        }

        drawingCtx.fillRect(
            *rt,
            window->x + right,
            window->y + top,
            window->x + right,
            window->y + top + 40,
            shade,
            Drawing::RectFlags::none);

        draw_resize_handle(rt, window, this, colour);
    }

    void Widget::draw_3(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        int16_t t, l, b, r;
        t = window->y + top;
        l = window->x + left;
        r = window->x + right;
        b = window->y + bottom;

        if (activated)
        {
            flags |= Drawing::RectInsetFlags::borderInset;
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (content == kContentUnk)
        {
            flags |= Drawing::RectInsetFlags::fillNone;
            drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags);
            return;
        }

        if (window->hasFlags(WindowFlags::flag_6))
        {
            drawingCtx.fillRect(*rt, l, t, r, b, enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);
        }

        drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags);

        if (content == kContentNull)
        {
            return;
        }

        sub_4CADE8(rt, window, colour, enabled, disabled, activated);
    }

    // 0x004CABFE
    void Widget::drawTab(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour, bool enabled, bool disabled, bool activated)
    {
        if (content == kContentNull)
        {
            return;
        }

        if (!disabled)
        {
            sub_4CADE8(rt, window, colour, enabled, disabled, activated);
            return;
        }

        if (type == WidgetType::tab)
        {
            return;
        }

        if (type != WidgetType::toolbarTab)
        {
            sub_4CADE8(rt, window, colour, enabled, disabled, activated);
            return;
        }

        const bool isColourSet = image & Widget::kImageIdColourSet;
        // TODO: Remove addedImage addition
        ImageId imageId = ImageId::fromUInt32(image & ~Widget::kImageIdColourSet).withIndexOffset(2);

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(colour.c());
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(*rt, Ui::Point(window->x + left, window->y + top), imageId);
    }

    // 0x004CACD4
    void Widget::drawButtonWithImage(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered)
    {
        if (!disabled && hovered)
        {
            // TODO: Fix mixed windows
            draw_3(rt, window, flags, colour, enabled, disabled, activated);
            return;
        }

        int l = left + window->x;
        int t = top + window->y;
        int r = right + window->x;
        int b = bottom + window->y;

        if (activated)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            flags |= Drawing::RectInsetFlags::borderInset;
            if (content == kContentUnk)
            {
                // 0x004CABE8

                flags |= Drawing::RectInsetFlags::fillNone;
                drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags);

                return;
            }

            drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags);
        }

        if (content == kContentNull)
        {
            return;
        }

        sub_4CADE8(rt, window, colour, enabled, disabled, activated);
    }

    // 0x004CAC5F
    void Widget::drawButtonWithColour(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour, bool enabled, [[maybe_unused]] bool disabled, bool activated, bool hovered)
    {
        if (content == kContentNull)
        {
            return;
        }

        const bool isColourSet = image & Widget::kImageIdColourSet;
        // TODO: Remove addition
        ImageId imageId = ImageId::fromUInt32(image & ~Widget::kImageIdColourSet);

        if (enabled && activated && hovered)
        {
            // TODO: Remove addition
            imageId = imageId.withIndexOffset(2);
        }
        else if (enabled && !activated && hovered)
        {
            // TODO: Remove addition
            imageId = imageId.withIndexOffset(1);
        }

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(colour.c());
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(*rt, Ui::Point(window->x + left, window->y + top), imageId);
    }

    // 0x004CB164
    void Widget::drawButton(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour, [[maybe_unused]] bool enabled, [[maybe_unused]] bool disabled, bool activated)
    {
        int l = window->x + left;
        int r = window->x + right;
        int t = window->y + top;
        int b = window->y + bottom;

        if (activated)
        {
            flags |= Drawing::RectInsetFlags::borderInset;
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags);
    }

    // 0x004CB1BE
    void Widget::draw_13(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour, [[maybe_unused]] bool enabled, bool disabled, bool activated)
    {
        if (content == kContentNull)
        {
            return;
        }

        int16_t x = window->x + left;
        int16_t y = window->y + std::max<int16_t>(top, (top + bottom) / 2 - 5);
        StringId string = text;

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
            draw_14(rt, this, colour, disabled, x, y, string);
        }
        else
        {
            draw_11_c(rt, window, this, colour, disabled, x, y, string);
        }
    }

    // 0x004CB21D
    void draw_11_c(Gfx::RenderTarget* rt, const Window* window, Widget* widget, AdvancedColour colour, bool disabled, [[maybe_unused]] int16_t x, int16_t y, StringId string)
    {
        colour = colour.opaque();
        if (disabled)
        {
            colour = colour.inset();
        }

        int16_t centreX = window->x + (widget->left + widget->right + 1) / 2 - 1;
        auto point = Point(centreX, y);
        int16_t width = widget->right - widget->left - 2;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringCentredClipped(*rt, point, width, colour, string, &FormatArguments::common());
    }

    // 0x004CB263
    void draw_14(Gfx::RenderTarget* rt, Widget* widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, StringId string)
    {
        x = x + 1;

        colour = colour.opaque();
        if (disabled)
        {
            colour = colour.inset();
        }

        auto point = Point(x, y);
        int width = widget->right - widget->left - 2;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringLeftClipped(*rt, point, width, colour, string, &FormatArguments::common());
    }

    // 0x4CB2D6
    void Widget::draw_15(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour, bool disabled)
    {
        if (content == kContentNull || content == kContentUnk)
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

        auto point = Point(window->x + left + 1, window->y + top);
        int width = this->right - this->left - 2;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringLeftClipped(*rt, point, width, colour, text, &FormatArguments::common());
    }

    // 0x4CB29C
    void Widget::drawTextBox(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(*rt, window->x + left, window->y + top, window->x + right, window->y + bottom, colour, flags | Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillDarker);
    }

    // 0x004CA6AE
    void Widget::draw_22_caption(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour)
    {
        int l = window->x + left;
        int r = window->x + right;
        int t = window->y + top;
        int b = window->y + bottom;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags | Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillDarker);
        drawingCtx.fillRect(*rt, l + 1, t + 1, r - 1, b - 1, enumValue(ExtColour::unk2E), Drawing::RectFlags::transparent);

        int16_t width = r - l - 4 - 10;
        auto point = Point(l + 2 + (width / 2), t + 1);

        drawingCtx.drawStringCentredClipped(*rt, point, width, AdvancedColour(Colour::white).outline(), text, &FormatArguments::common());
    }

    // 0x004CA750
    void Widget::draw_23_caption(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::Colour::black;
        StringManager::formatString(&stringBuffer[1], text, &FormatArguments::common());

        int16_t width = right - left - 4 - 14;
        int16_t x = left + window->x + 2 + (width / 2);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        width = drawingCtx.clipString(width - 8, stringBuffer);

        x -= width / 2;
        int16_t y = window->y + top + 1;

        drawStationNameBackground(rt, window, this, x, y, colour, width);

        auto point = Point(x, y);
        drawingCtx.drawString(*rt, point, Colour::black, stringBuffer);
    }

    // 0x004CA7F6
    void Widget::draw_24_caption(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, [[maybe_unused]] AdvancedColour colour)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::windowColour1;
        StringManager::formatString(&stringBuffer[1], text, &FormatArguments::common());

        int16_t x = left + window->x + 2;
        int16_t width = right - left - 4 - 14;
        x = x + (width / 2);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        int16_t stringWidth = drawingCtx.clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        auto point = Point(x, window->y + top + 1);
        drawingCtx.drawString(*rt, point, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    // 0x004CA88B
    void Widget::draw_25_caption(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, [[maybe_unused]] AdvancedColour colour)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::Colour::white;
        StringManager::formatString(&stringBuffer[1], text, &FormatArguments::common());

        int16_t x = left + window->x + 2;
        int16_t width = right - left - 4 - 14;
        x = x + (width / 2);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        int16_t stringWidth = drawingCtx.clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        auto point = Point(x, window->y + top + 1);
        drawingCtx.drawString(*rt, point, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    static void draw_hscroll(Gfx::RenderTarget* rt, const Window* window, Widget* widget, Drawing::RectInsetFlags flags, AdvancedColour colour, [[maybe_unused]] bool enabled, [[maybe_unused]] bool disabled, [[maybe_unused]] bool activated, [[maybe_unused]] bool hovered, int16_t scrollview_index)
    {
        const auto* scroll_area = &window->scrollAreas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        cx = dx - 10;
        if (scroll_area->hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            bx -= 11;
        }

        Drawing::RectInsetFlags f;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // pusha
        f = Drawing::RectInsetFlags::none;
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarLeftPressed))
        {
            f = flags | Drawing::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, cx, ax + 9, dx, colour, f);
        // popa

        // pusha
        {
            auto point = Point(ax + 2, cx);
            const char* hLeftStr = "\x90\xBE";
            drawingCtx.drawString(*rt, point, Colour::black, hLeftStr);
        }
        // popa

        // pusha
        f = Drawing::RectInsetFlags::none;
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarRightPressed))
        {
            f = flags | Drawing::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, bx - 9, cx, bx, dx, colour, f);
        // popa

        // pusha
        {
            auto point = Point(bx - 7, cx);
            const char* hRightStr = "\x90\xAF";
            drawingCtx.drawString(*rt, point, Colour::black, hRightStr);
        }
        // popa

        // pusha
        drawingCtx.fillRect(*rt, ax + 10, cx, bx - 10, dx, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx, bx - 10, dx, Colours::getShade(colour.c(), 3), Drawing::RectFlags::crossHatching);
        // popa

        // pusha
        drawingCtx.fillRect(*rt, ax + 10, cx + 2, bx - 10, cx + 2, Colours::getShade(colour.c(), 3), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx + 3, bx - 10, cx + 3, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx + 7, bx - 10, cx + 7, Colours::getShade(colour.c(), 3), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx + 8, bx - 10, cx + 8, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
        // popa

        // pusha
        f = Drawing::RectInsetFlags::none;
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarThumbPressed))
        {
            f = Drawing::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax - 1 + scroll_area->hThumbLeft, cx, ax - 1 + scroll_area->hThumbRight, dx, colour, f);
        // popa
    }

    static void draw_vscroll(Gfx::RenderTarget* rt, const Window* window, Widget* widget, Drawing::RectInsetFlags flags, AdvancedColour colour, [[maybe_unused]] bool enabled, [[maybe_unused]] bool disabled, [[maybe_unused]] bool activated, [[maybe_unused]] bool hovered, int16_t scrollview_index)
    {
        const auto* scroll_area = &window->scrollAreas[scrollview_index];

        uint16_t ax = window->x + widget->left + 1;
        uint16_t cx = window->y + widget->top + 1;
        uint16_t bx = window->x + widget->right - 1;
        uint16_t dx = window->y + widget->bottom - 1;

        ax = bx - 10;
        if (scroll_area->hasFlags(ScrollFlags::hscrollbarVisible))
        {
            dx -= 11;
        }

        Drawing::RectInsetFlags f = Drawing::RectInsetFlags::none;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // pusha
        if (scroll_area->hasFlags(ScrollFlags::vscrollbarUpPressed))
        {
            f = flags | Drawing::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, cx, bx, cx + 9, colour, f);
        // popa

        // pusha
        {
            auto point = Point(ax + 1, cx - 1);
            const char* vTopStr = "\x90\xA0";
            drawingCtx.drawString(*rt, point, Colour::black, vTopStr);
        }
        // popa

        // pusha
        f = Drawing::RectInsetFlags::none;
        if (scroll_area->hasFlags(ScrollFlags::vscrollbarDownPressed))
        {
            f = flags | Drawing::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, dx - 9, bx, dx, colour, f);
        // popa

        // pusha
        {
            auto point = Point(ax + 1, dx - 9);
            const char* vBottomStr = "\x90\xAA";
            drawingCtx.drawString(*rt, point, Colour::black, vBottomStr);
        }
        // popa

        // pusha
        drawingCtx.fillRect(*rt, ax, cx + 10, bx, dx - 10, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax, cx + 10, bx, dx - 10, Colours::getShade(colour.c(), 3), Drawing::RectFlags::crossHatching);
        // popa

        // pusha
        drawingCtx.fillRect(*rt, ax + 2, cx + 10, ax + 2, dx - 10, Colours::getShade(colour.c(), 3), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 3, cx + 10, ax + 3, dx - 10, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 7, cx + 10, ax + 7, dx - 10, Colours::getShade(colour.c(), 3), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 8, cx + 10, ax + 8, dx - 10, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
        // popa

        // pusha
        f = Drawing::RectInsetFlags::none;
        if (scroll_area->hasFlags(ScrollFlags::vscrollbarThumbPressed))
        {
            f = flags | Drawing::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, cx - 1 + scroll_area->vThumbTop, bx, cx - 1 + scroll_area->vThumbBottom, colour, f);
        // popa
    }

    // 0x004CB31C
    void Widget::drawScrollview(Gfx::RenderTarget* rt, Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index)
    {
        int16_t l = window->x + left;
        int16_t t = window->y + top;
        int16_t r = window->x + right;
        int16_t b = window->y + bottom;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(*rt, l, t, r, b, colour, flags | Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillDarker);

        l++;
        t++;
        r--;
        b--;

        const auto* scroll_area = &window->scrollAreas[scrollview_index];

        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarVisible))
        {
            draw_hscroll(rt, window, this, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            b -= 11;
        }

        if (scroll_area->hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            draw_vscroll(rt, window, this, flags, colour, enabled, disabled, activated, hovered, scrollview_index);
            r -= 11;
        }

        Gfx::RenderTarget cropped = *rt;
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
    void Widget::draw_27_checkbox(Gfx::RenderTarget* rt, const Window* window, Drawing::RectInsetFlags flags, AdvancedColour colour, bool enabled, [[maybe_unused]] bool disabled, bool activated)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (enabled)
        {
            drawingCtx.fillRectInset(
                *rt,
                window->x + left,
                window->y + top,
                window->x + left + 9,
                window->y + bottom - 1,
                colour,
                flags | Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillDarker);
        }

        if (activated)
        {
            static constexpr char strCheckmark[] = "\xAC";
            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
            drawingCtx.drawString(*rt, window->x + left, window->y + top, colour.opaque(), strCheckmark);
        }
    }

    // 0x004CB080
    void Widget::draw_27_label(Gfx::RenderTarget* rt, const Window* window, [[maybe_unused]] Drawing::RectInsetFlags flags, AdvancedColour colour, bool disabled)
    {
        if (content == kContentNull)
        {
            return;
        }

        colour = colour.opaque();

        if (disabled)
        {
            colour = colour.inset();
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringLeft(*rt, window->x + left + 14, window->y + top, colour, text, &FormatArguments::common());
    }

    // 0x004CA679
    void Widget::draw_29(Gfx::RenderTarget* rt, const Window* window)
    {
        int l = window->x + left;
        int r = window->x + right;
        int t = window->y + top;
        int b = window->y + bottom;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRect(*rt, l, t, r, b, Colours::getShade(Colour::black, 5), Drawing::RectFlags::none);
    }

    void Widget::drawGroupbox(Gfx::RenderTarget* const rt, const Window* window)
    {
        const auto colour = window->getColour(windowColour).opaque();
        int32_t l = window->x + left + 5;
        int32_t t = window->y + top;
        int32_t r = window->x + right;
        int32_t b = window->y + bottom;
        int32_t textEndPos = l;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        // First, draw the label text, if any.
        if (text != StringIds::null)
        {
            char buffer[512] = { 0 };
            StringManager::formatString(buffer, sizeof(buffer), text);

            drawingCtx.drawString(*rt, l, t, colour, buffer);
            textEndPos = l + drawingCtx.getStringWidth(buffer) + 1;
        }

        // Prepare border dimensions
        l = window->x + left;
        t = window->y + top + 4;
        r = window->x + right;
        b = window->y + bottom;

        // Border left of text
        drawingCtx.fillRect(*rt, l, t, l + 4, t, Colours::getShade(colour.c(), 4), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, l + 1, t + 1, l + 4, t + 1, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);

        // Border right of text
        drawingCtx.fillRect(*rt, textEndPos, t, r - 1, t, Colours::getShade(colour.c(), 4), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, textEndPos, t + 1, r - 2, t + 1, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);

        // Border right
        drawingCtx.fillRect(*rt, r - 1, t + 1, r - 1, b - 1, Colours::getShade(colour.c(), 4), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, r, t, r, b, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);

        // Border bottom
        drawingCtx.fillRect(*rt, l, b - 1, r - 2, b - 1, Colours::getShade(colour.c(), 4), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, l, b, r - 1, b, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);

        // Border left
        drawingCtx.fillRect(*rt, l, t + 1, l, b - 2, Colours::getShade(colour.c(), 4), Drawing::RectFlags::none);
        drawingCtx.fillRect(*rt, l + 1, t + 2, l + 1, b - 2, Colours::getShade(colour.c(), 7), Drawing::RectFlags::none);
    }

    // 0x004CF194
    void Widget::drawTab(Window* w, Gfx::RenderTarget* rt, uint32_t imageId, WidgetIndex_t index)
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

        if (imageId == kContentNull)
        {
            return;
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (isActivated)
        {
            if (imageId != kContentUnk)
            {
                drawingCtx.drawImage(rt, pos.x, pos.y, imageId);
            }
        }
        else
        {
            if (imageId != kContentUnk)
            {
                drawingCtx.drawImage(rt, pos.x, pos.y + 1, imageId);
            }

            drawingCtx.drawImage(rt, pos.x, pos.y, Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33));
            drawingCtx.drawRect(*rt, pos.x, pos.y + 26, 31, 1, Colours::getShade(w->getColour(WindowColour::secondary).c(), 7), Drawing::RectFlags::none);
        }
    }

    void Widget::leftAlignTabs(Window& window, uint8_t firstTabIndex, uint8_t lastTabIndex, uint16_t tabWidth)
    {
        auto xPos = window.widgets[firstTabIndex].left;
        for (auto i = firstTabIndex; i <= lastTabIndex; i++)
        {
            if (window.isDisabled(i))
            {
                window.widgets[i].type = WidgetType::none;
            }

            else
            {
                window.widgets[i].type = WidgetType::tab;
                window.widgets[i].left = xPos;
                window.widgets[i].right = xPos + tabWidth;
                xPos = window.widgets[i].right + 1;
            }
        }
    }
}
