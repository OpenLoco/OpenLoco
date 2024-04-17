#include "Widget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
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

    static void drawPanel(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawFrame(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_3(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawTabImpl(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawButtonWithImage(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawButtonWithColour(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawButton(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_11_c(Gfx::RenderTarget* rt, const Window* window, const Widget& widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, StringId string);
    static void draw_13(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_14(Gfx::RenderTarget* rt, const Widget& widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, StringId string);
    static void draw_15(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawTextBox(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_22_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_23_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_24_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_25_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawScrollview(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_27_checkbox(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_27_label(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void draw_29(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawGroupbox(Gfx::RenderTarget* const rt, const Widget& widget, const WidgetState& widgetState);
    static void drawViewports(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);
    static void drawViewportCentreButton(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState);

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

        Gfx::RectInsetFlags widgetFlags = Gfx::RectInsetFlags::none;
        if (windowColour == WindowColour::primary && window->hasFlags(WindowFlags::flag_11))
        {
            widgetFlags = Gfx::RectInsetFlags::colourLight;
        }

        const auto widgetIndex = this - &window->widgets[0];
        WidgetState widgetState{};

        widgetState.window = window;
        widgetState.flags = widgetFlags;
        widgetState.colour = window->getColour(windowColour);
        widgetState.enabled = (window->enabledWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.disabled = (window->disabledWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.activated = (window->activatedWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.activated |= (pressedWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.activated |= (toolWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.hovered = (hoveredWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.scrollviewIndex = scrollviewIndex;

        switch (type)
        {
            case WidgetType::none:
            case WidgetType::end:
                break;

            case WidgetType::panel:
                drawPanel(rt, *this, widgetState);
                break;

            case WidgetType::frame:
                drawFrame(rt, *this, widgetState);
                break;

            case WidgetType::wt_3:
                draw_3(rt, *this, widgetState);
                break;

            case WidgetType::wt_4:
                assert(false); // Unused
                break;

            case WidgetType::slider:
            case WidgetType::wt_6:
            case WidgetType::toolbarTab:
            case WidgetType::tab:
                drawTabImpl(rt, *this, widgetState);
                break;

            case WidgetType::buttonWithImage:
                drawButtonWithImage(rt, *this, widgetState);
                break;

            case WidgetType::buttonWithColour:
                drawButtonWithColour(rt, *this, widgetState);
                break;

            case WidgetType::button:
            case WidgetType::wt_12:
            case WidgetType::buttonTableHeader:
                if (type == WidgetType::wt_12)
                {
                    assert(false); // Unused
                }
                drawButton(rt, *this, widgetState);
                draw_13(rt, *this, widgetState);
                break;

            case WidgetType::wt_13:
                draw_13(rt, *this, widgetState);
                break;

            case WidgetType::wt_15:
                draw_15(rt, *this, widgetState);
                break;

            case WidgetType::groupbox:
                // NB: widget type 16 has been repurposed to add groupboxes; the original type 16 was unused.
                drawGroupbox(rt, *this, widgetState);
                break;

            case WidgetType::textbox:
            case WidgetType::combobox:
                drawTextBox(rt, *this, widgetState);
                draw_15(rt, *this, widgetState);
                break;
            case WidgetType::viewport:
                drawTextBox(rt, *this, widgetState);
                draw_15(rt, *this, widgetState);
                drawViewports(rt, *this, widgetState);
                break;
            case WidgetType::wt_20:
            case WidgetType::wt_21:
                assert(false); // Unused
                break;

            case WidgetType::caption_22:
                draw_22_caption(rt, *this, widgetState);
                break;

            case WidgetType::caption_23:
                draw_23_caption(rt, *this, widgetState);
                break;

            case WidgetType::caption_24:
                draw_24_caption(rt, *this, widgetState);
                break;

            case WidgetType::caption_25:
                draw_25_caption(rt, *this, widgetState);
                break;

            case WidgetType::scrollview:
                drawScrollview(rt, *this, widgetState);
                scrollviewIndex++;
                break;

            case WidgetType::checkbox:
                draw_27_checkbox(rt, *this, widgetState);
                draw_27_label(rt, *this, widgetState);
                break;

            case WidgetType::wt_28:
                assert(false); // Unused
                draw_27_label(rt, *this, widgetState);
                break;

            case WidgetType::wt_29:
                assert(false); // Unused
                draw_29(rt, *this, widgetState);
                break;
            case WidgetType::viewportCentreButton:
                drawViewportCentreButton(rt, *this, widgetState);
                break;
        }
    }

    // 0x004CF487
    static void drawViewportCentreButton(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        if (widgetState.hovered)
        {
            drawingCtx.drawRect(*rt, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), enumValue(ExtColour::translucentGrey2), Gfx::RectFlags::transparent);
            drawingCtx.drawRect(*rt, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), enumValue(ExtColour::unk34), Gfx::RectFlags::transparent);

            Gfx::RectInsetFlags flags = Gfx::RectInsetFlags::none;
            if (widgetState.activated)
                flags = Gfx::RectInsetFlags::borderInset;

            drawingCtx.drawRectInset(*rt, widget.left + window->x, widget.top + window->y, widget.width(), widget.height(), window->getColour(WindowColour::secondary).translucent(), flags);
        }

        drawingCtx.drawImage(rt, widget.left + window->x, widget.top + window->y, Gfx::recolour(ImageIds::centre_viewport, window->getColour(WindowColour::secondary).c()));
    }

    // 0x004CAB8E
    static void draw_resize_handle(Gfx::RenderTarget* rt, const Window* window, const Widget& widget, AdvancedColour colour)
    {
        if (!window->hasFlags(WindowFlags::resizable))
        {
            return;
        }

        if (window->minHeight == window->maxHeight || window->minWidth == window->maxWidth)
        {
            return;
        }

        int16_t x = widget.right + window->x - 18;
        int16_t y = widget.bottom + window->y - 18;
        uint32_t image = Gfx::recolour(ImageIds::window_resize_handle, colour.c());
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(rt, x, y, image);
    }

    static void sub_4CADE8(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        Ui::Point placeForImage(widget.left + window->x, widget.top + window->y);
        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet);
        if (widget.type == WidgetType::wt_6 || widget.type == WidgetType::toolbarTab || widget.type == WidgetType::tab || widget.type == WidgetType::wt_4)
        {
            if (widgetState.activated)
            {
                // TODO: remove image addition
                imageId = imageId.withIndexOffset(1);
            }
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto colour = widgetState.colour;
        if (widgetState.disabled)
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
    static void drawPanel(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(
            *rt,
            window->x + widget.left,
            window->y + widget.top,
            window->x + widget.right,
            window->y + widget.bottom,
            widgetState.colour,
            widgetState.flags);

        draw_resize_handle(rt, window, widget, widgetState.colour);
    }

    // 0x004CAAB9
    static void drawFrame(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(widget.left + window->x, widget.top + window->y, widget.right - widget.left, 41));
        if (clipped)
        {
            uint32_t imageId = widget.image;
            if (window->hasFlags(WindowFlags::flag_11))
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image, widgetState.colour.c());
            }
            else
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image_alt, widgetState.colour.c());
            }
            drawingCtx.drawImage(&*clipped, 0, 0, imageId);
        }

        uint8_t shade;
        if (window->hasFlags(WindowFlags::flag_11))
        {
            shade = Colours::getShade(widgetState.colour.c(), 3);
        }
        else
        {
            shade = Colours::getShade(widgetState.colour.c(), 1);
        }

        drawingCtx.fillRect(
            *rt,
            window->x + widget.right,
            window->y + widget.top,
            window->x + widget.right,
            window->y + widget.top + 40,
            shade,
            Gfx::RectFlags::none);

        draw_resize_handle(rt, window, widget, widgetState.colour);
    }

    static void draw_3(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        int16_t t, l, b, r;
        t = window->y + widget.top;
        l = window->x + widget.left;
        r = window->x + widget.right;
        b = window->y + widget.bottom;

        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (widget.content == Widget::kContentUnk)
        {
            flags |= Gfx::RectInsetFlags::fillNone;
            drawingCtx.fillRectInset(*rt, l, t, r, b, widgetState.colour, flags);
            return;
        }

        if (window->hasFlags(WindowFlags::flag_6))
        {
            drawingCtx.fillRect(*rt, l, t, r, b, enumValue(ExtColour::unk34), Gfx::RectFlags::transparent);
        }

        drawingCtx.fillRectInset(*rt, l, t, r, b, widgetState.colour, flags);

        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        sub_4CADE8(rt, widget, widgetState);
    }

    // 0x004CABFE
    static void drawTabImpl(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        if (!widgetState.disabled)
        {
            sub_4CADE8(rt, widget, widgetState);
            return;
        }

        if (widget.type == WidgetType::tab)
        {
            return;
        }

        if (widget.type != WidgetType::toolbarTab)
        {
            sub_4CADE8(rt, widget, widgetState);
            return;
        }

        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        // TODO: Remove addedImage addition
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet).withIndexOffset(2);

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(widgetState.colour.c());
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(*rt, Ui::Point(window->x + widget.left, window->y + widget.top), imageId);
    }

    // 0x004CACD4
    static void drawButtonWithImage(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        if (!widgetState.disabled && widgetState.hovered)
        {
            // TODO: Fix mixed windows
            draw_3(rt, widget, widgetState);
            return;
        }

        auto* window = widgetState.window;
        int l = widget.left + window->x;
        int t = widget.top + window->y;
        int r = widget.right + window->x;
        int b = widget.bottom + window->y;

        if (widgetState.activated)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            auto flags = widgetState.flags;
            flags |= Gfx::RectInsetFlags::borderInset;
            if (widget.content == Widget::kContentUnk)
            {
                // 0x004CABE8

                flags |= Gfx::RectInsetFlags::fillNone;
                drawingCtx.fillRectInset(*rt, l, t, r, b, widgetState.colour, flags);

                return;
            }

            drawingCtx.fillRectInset(*rt, l, t, r, b, widgetState.colour, flags);
        }

        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        sub_4CADE8(rt, widget, widgetState);
    }

    // 0x004CAC5F
    static void drawButtonWithColour(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        // TODO: Remove addition
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet);

        if (widgetState.enabled && widgetState.activated && widgetState.hovered)
        {
            // TODO: Remove addition
            imageId = imageId.withIndexOffset(2);
        }
        else if (widgetState.enabled && !widgetState.activated && widgetState.hovered)
        {
            // TODO: Remove addition
            imageId = imageId.withIndexOffset(1);
        }

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(widgetState.colour.c());
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto* window = widgetState.window;
        drawingCtx.drawImage(*rt, Ui::Point(window->x + widget.left, window->y + widget.top), imageId);
    }

    // 0x004CB164
    static void drawButton(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        int l = window->x + widget.left;
        int r = window->x + widget.right;
        int t = window->y + widget.top;
        int b = window->y + widget.bottom;

        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(*rt, l, t, r, b, widgetState.colour, flags);
    }

    // 0x004CB1BE
    static void draw_13(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        auto* window = widgetState.window;
        int16_t x = window->x + widget.left;
        int16_t y = window->y + std::max<int16_t>(widget.top, (widget.top + widget.bottom) / 2 - 5);
        StringId string = widget.text;

        // TODO: Refactor out Widget type check
        if (widget.type == WidgetType::wt_12)
        {
            if (widgetState.activated)
            {
                // TODO: Remove string addition
                string++;
            }
        }

        if (widget.type == WidgetType::buttonTableHeader)
        {
            draw_14(rt, widget, widgetState.colour, widgetState.disabled, x, y, string);
        }
        else
        {
            draw_11_c(rt, window, widget, widgetState.colour, widgetState.disabled, x, y, string);
        }
    }

    // 0x004CB21D
    static void draw_11_c(Gfx::RenderTarget* rt, const Window* window, const Widget& widget, AdvancedColour colour, bool disabled, [[maybe_unused]] int16_t x, int16_t y, StringId string)
    {
        colour = colour.opaque();
        if (disabled)
        {
            colour = colour.inset();
        }

        int16_t centreX = window->x + (widget.left + widget.right + 1) / 2 - 1;
        int16_t width = widget.right - widget.left - 2;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringCentredClipped(*rt, Point(centreX, y), width, colour, string, &FormatArguments::common());
    }

    // 0x004CB263
    static void draw_14(Gfx::RenderTarget* rt, const Widget& widget, AdvancedColour colour, bool disabled, int16_t x, int16_t y, StringId string)
    {
        x = x + 1;

        colour = colour.opaque();
        if (disabled)
        {
            colour = colour.inset();
        }

        int width = widget.right - widget.left - 2;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringLeftClipped(*rt, Point(x, y), width, colour, string, &FormatArguments::common());
    }

    // 0x4CB2D6
    static void draw_15(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetStated)
    {
        if (widget.content == Widget::kContentNull || widget.content == Widget::kContentUnk)
        {
            return;
        }

        auto colour = widgetStated.colour;
        if (widgetStated.disabled)
        {
            colour = colour.inset();
        }
        else
        {
            colour = colour.FD();
        }

        auto* window = widgetStated.window;
        auto point = Point(window->x + widget.left + 1, window->y + widget.top);
        int width = widget.right - widget.left - 2;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringLeftClipped(*rt, point, width, colour, widget.text, &FormatArguments::common());
    }

    // 0x4CB29C
    static void drawTextBox(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        const auto flags = widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker;
        drawingCtx.fillRectInset(
            *rt,
            window->x + widget.left,
            window->y + widget.top,
            window->x + widget.right,
            window->y + widget.bottom,
            widgetState.colour,
            flags);
    }

    // 0x004CA6AE
    static void draw_22_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        int l = window->x + widget.left;
        int r = window->x + widget.right;
        int t = window->y + widget.top;
        int b = window->y + widget.bottom;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.fillRectInset(
            *rt,
            l,
            t,
            r,
            b,
            widgetState.colour,
            widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        drawingCtx.fillRect(
            *rt,
            l + 1,
            t + 1,
            r - 1,
            b - 1,
            enumValue(ExtColour::unk2E),
            Gfx::RectFlags::transparent);

        int16_t width = r - l - 4 - 10;
        auto point = Point(l + 2 + (width / 2), t + 1);

        auto formatArgs = FormatArguments(widget.textArgs);
        drawingCtx.drawStringCentredClipped(
            *rt,
            point,
            width,
            AdvancedColour(Colour::white).outline(),
            widget.text,
            &formatArgs);
    }

    // 0x004CF3EB
    static void drawStationNameBackground(Gfx::RenderTarget* rt, [[maybe_unused]] const Window* window, [[maybe_unused]] const Widget* widget, int16_t x, int16_t y, AdvancedColour colour, int16_t width)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(rt, x - 4, y, Gfx::recolour(ImageIds::curved_border_left_medium, colour.c()));
        drawingCtx.drawImage(rt, x + width, y, Gfx::recolour(ImageIds::curved_border_right_medium, colour.c()));
        drawingCtx.fillRect(*rt, x, y, x + width - 1, y + 11, Colours::getShade(colour.c(), 5), Gfx::RectFlags::none);
    }

    // 0x004CA750
    static void draw_23_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto formatArgs = FormatArguments(widget.textArgs);

        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::Colour::black;
        StringManager::formatString(&stringBuffer[1], widget.text, &formatArgs);

        auto* window = widgetState.window;
        int16_t width = widget.right - widget.left - 4 - 14;
        int16_t x = widget.left + window->x + 2 + (width / 2);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        width = drawingCtx.clipString(width - 8, stringBuffer);

        x -= width / 2;
        int16_t y = window->y + widget.top + 1;

        drawStationNameBackground(rt, window, &widget, x, y, widgetState.colour, width);

        drawingCtx.drawString(*rt, Point(x, y), Colour::black, stringBuffer);
    }

    // 0x004CA7F6
    static void draw_24_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto formatArgs = FormatArguments(widget.textArgs);

        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::windowColour1;
        StringManager::formatString(&stringBuffer[1], widget.text, &formatArgs);

        auto* window = widgetState.window;
        int16_t x = widget.left + window->x + 2;
        int16_t width = widget.right - widget.left - 4 - 14;
        x = x + (width / 2);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        int16_t stringWidth = drawingCtx.clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        auto point = Point(x, window->y + widget.top + 1);
        drawingCtx.drawString(*rt, point, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    // 0x004CA88B
    static void draw_25_caption(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        char stringBuffer[512];
        stringBuffer[0] = ControlCodes::Colour::white;
        StringManager::formatString(&stringBuffer[1], widget.text, &FormatArguments::common());

        auto* window = widgetState.window;
        int16_t x = widget.left + window->x + 2;
        int16_t width = widget.right - widget.left - 4 - 14;
        x = x + (width / 2);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        int16_t stringWidth = drawingCtx.clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        auto point = Point(x, window->y + widget.top + 1);
        drawingCtx.drawString(*rt, point, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    static void draw_hscroll(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        const auto* scroll_area = &window->scrollAreas[widgetState.scrollviewIndex];

        uint16_t ax = window->x + widget.left + 1;
        uint16_t cx = window->y + widget.top + 1;
        uint16_t bx = window->x + widget.right - 1;
        uint16_t dx = window->y + widget.bottom - 1;

        cx = dx - 10;
        if (scroll_area->hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            bx -= 11;
        }

        Gfx::RectInsetFlags f;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarLeftPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, cx, ax + 9, dx, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* hLeftStr = "\x90\xBE";
            drawingCtx.drawString(*rt, Point(ax + 2, cx), Colour::black, hLeftStr);
        }
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarRightPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, bx - 9, cx, bx, dx, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* hRightStr = "\x90\xAF";
            drawingCtx.drawString(*rt, Point(bx - 7, cx), Colour::black, hRightStr);
        }
        // popa

        const auto colour = widgetState.colour;
        // pusha
        drawingCtx.fillRect(*rt, ax + 10, cx, bx - 10, dx, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx, bx - 10, dx, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);
        // popa

        // pusha
        drawingCtx.fillRect(*rt, ax + 10, cx + 2, bx - 10, cx + 2, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx + 3, bx - 10, cx + 3, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx + 7, bx - 10, cx + 7, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 10, cx + 8, bx - 10, cx + 8, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarThumbPressed))
        {
            f = Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax - 1 + scroll_area->hThumbLeft, cx, ax - 1 + scroll_area->hThumbRight, dx, colour, f);
        // popa
    }

    static void draw_vscroll(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        const auto* scroll_area = &window->scrollAreas[widgetState.scrollviewIndex];

        uint16_t ax = window->x + widget.left + 1;
        uint16_t cx = window->y + widget.top + 1;
        uint16_t bx = window->x + widget.right - 1;
        uint16_t dx = window->y + widget.bottom - 1;

        ax = bx - 10;
        if (scroll_area->hasFlags(ScrollFlags::hscrollbarVisible))
        {
            dx -= 11;
        }

        Gfx::RectInsetFlags f = Gfx::RectInsetFlags::none;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // pusha
        if (scroll_area->hasFlags(ScrollFlags::vscrollbarUpPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, cx, bx, cx + 9, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* vTopStr = "\x90\xA0";
            drawingCtx.drawString(*rt, Point(ax + 1, cx - 1), Colour::black, vTopStr);
        }
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scroll_area->hasFlags(ScrollFlags::vscrollbarDownPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, dx - 9, bx, dx, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* vBottomStr = "\x90\xAA";
            drawingCtx.drawString(*rt, Point(ax + 1, dx - 9), Colour::black, vBottomStr);
        }
        // popa

        const auto colour = widgetState.colour;
        // pusha
        drawingCtx.fillRect(*rt, ax, cx + 10, bx, dx - 10, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax, cx + 10, bx, dx - 10, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);
        // popa

        // pusha
        drawingCtx.fillRect(*rt, ax + 2, cx + 10, ax + 2, dx - 10, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 3, cx + 10, ax + 3, dx - 10, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 7, cx + 10, ax + 7, dx - 10, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, ax + 8, cx + 10, ax + 8, dx - 10, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scroll_area->hasFlags(ScrollFlags::vscrollbarThumbPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(*rt, ax, cx - 1 + scroll_area->vThumbTop, bx, cx - 1 + scroll_area->vThumbBottom, colour, f);
        // popa
    }

    // 0x004CB31C
    static void drawScrollview(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        int16_t l = window->x + widget.left;
        int16_t t = window->y + widget.top;
        int16_t r = window->x + widget.right;
        int16_t b = window->y + widget.bottom;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRectInset(*rt, l, t, r, b, widgetState.colour, widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        l++;
        t++;
        r--;
        b--;

        const auto* scroll_area = &window->scrollAreas[widgetState.scrollviewIndex];

        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        if (scroll_area->hasFlags(Ui::ScrollFlags::hscrollbarVisible))
        {
            draw_hscroll(rt, widget, widgetState);
            b -= 11;
        }

        if (scroll_area->hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            draw_vscroll(rt, widget, widgetState);
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

            window->callDrawScroll(&cropped, widgetState.scrollviewIndex);
        }
    }

    // 0x004CB00B
    static void draw_27_checkbox(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (widgetState.enabled)
        {
            drawingCtx.fillRectInset(
                *rt,
                window->x + widget.left,
                window->y + widget.top,
                window->x + widget.left + 9,
                window->y + widget.bottom - 1,
                widgetState.colour,
                widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);
        }

        if (widgetState.activated)
        {
            static constexpr char strCheckmark[] = "\xAC";
            auto point = Point(window->x + widget.left, window->y + widget.top);

            auto color = widgetState.colour;
            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
            drawingCtx.drawString(*rt, point, color.opaque(), strCheckmark);
        }
    }

    // 0x004CB080
    static void draw_27_label(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        auto colour = widgetState.colour;
        colour = colour.opaque();

        if (widgetState.disabled)
        {
            colour = colour.inset();
        }

        auto* window = widgetState.window;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto point = Point(window->x + widget.left + 14, window->y + widget.top);
        drawingCtx.drawStringLeft(*rt, point, colour, widget.text, &FormatArguments::common());
    }

    // 0x004CA679
    static void draw_29(Gfx::RenderTarget* rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        int l = window->x + widget.left;
        int r = window->x + widget.right;
        int t = window->y + widget.top;
        int b = window->y + widget.bottom;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRect(*rt, l, t, r, b, Colours::getShade(Colour::black, 5), Gfx::RectFlags::none);
    }

    static void drawGroupbox(Gfx::RenderTarget* const rt, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        auto colour = widgetState.colour;
        colour = colour.opaque();
        int32_t l = window->x + widget.left + 5;
        int32_t t = window->y + widget.top;
        int32_t r = window->x + widget.right;
        int32_t b = window->y + widget.bottom;
        int32_t textEndPos = l;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        // First, draw the label text, if any.
        if (widget.text != StringIds::null)
        {
            char buffer[512] = { 0 };
            StringManager::formatString(buffer, sizeof(buffer), widget.text);

            auto point = Point(l, t);
            drawingCtx.drawString(*rt, point, colour, buffer);
            textEndPos = l + drawingCtx.getStringWidth(buffer) + 1;
        }

        // Prepare border dimensions
        l = window->x + widget.left;
        t = window->y + widget.top + 4;
        r = window->x + widget.right;
        b = window->y + widget.bottom;

        // Border left of text
        drawingCtx.fillRect(*rt, l, t, l + 4, t, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, l + 1, t + 1, l + 4, t + 1, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border right of text
        drawingCtx.fillRect(*rt, textEndPos, t, r - 1, t, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, textEndPos, t + 1, r - 2, t + 1, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border right
        drawingCtx.fillRect(*rt, r - 1, t + 1, r - 1, b - 1, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, r, t, r, b, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border bottom
        drawingCtx.fillRect(*rt, l, b - 1, r - 2, b - 1, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, l, b, r - 1, b, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border left
        drawingCtx.fillRect(*rt, l, t + 1, l, b - 2, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(*rt, l + 1, t + 2, l + 1, b - 2, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
    }

    // 0x0045A0B3
    static void drawViewports(Gfx::RenderTarget* rt, [[maybe_unused]] const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        // TODO: Move viewports into the Widget
        auto& viewports = window->viewports;

        if (viewports[0] != nullptr)
            viewports[0]->render(rt);

        if (viewports[1] != nullptr)
            viewports[1]->render(rt);
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
            drawingCtx.drawRect(*rt, pos.x, pos.y + 26, 31, 1, Colours::getShade(w->getColour(WindowColour::secondary).c(), 7), Gfx::RectFlags::none);
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
