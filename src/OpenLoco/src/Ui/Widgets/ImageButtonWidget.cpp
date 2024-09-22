#include "ImageButtonWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "LabelWidget.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CADE8
    static void sub_4CADE8(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
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
                drawingCtx.drawImageSolid(placeForImage + Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 2);
                drawingCtx.drawImageSolid(placeForImage, pureImage, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                drawingCtx.drawImageSolid(placeForImage + Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(placeForImage, pureImage, c);
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

        drawingCtx.drawImage(placeForImage, imageId);
    }

    static void draw_3(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
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

        if (widget.content == Widget::kContentUnk)
        {
            flags |= Gfx::RectInsetFlags::fillNone;
            drawingCtx.fillRectInset(l, t, r, b, widgetState.colour, flags);
            return;
        }

        if (window->hasFlags(WindowFlags::flag_6))
        {
            drawingCtx.fillRect(l, t, r, b, enumValue(ExtColour::unk34), Gfx::RectFlags::transparent);
        }

        drawingCtx.fillRectInset(l, t, r, b, widgetState.colour, flags);

        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        sub_4CADE8(drawingCtx, widget, widgetState);
    }

    // 0x004CB164
    void ImageButton::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (!widgetState.disabled && widgetState.hovered)
        {
            // TODO: Fix mixed windows
            draw_3(drawingCtx, widget, widgetState);
            return;
        }

        auto* window = widgetState.window;
        int l = widget.left + window->x;
        int t = widget.top + window->y;
        int r = widget.right + window->x;
        int b = widget.bottom + window->y;

        if (widgetState.activated)
        {
            auto flags = widgetState.flags;
            flags |= Gfx::RectInsetFlags::borderInset;
            if (widget.content == Widget::kContentUnk)
            {
                // 0x004CABE8

                flags |= Gfx::RectInsetFlags::fillNone;
                drawingCtx.fillRectInset(l, t, r, b, widgetState.colour, flags);

                return;
            }

            drawingCtx.fillRectInset(l, t, r, b, widgetState.colour, flags);
        }

        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        sub_4CADE8(drawingCtx, widget, widgetState);
    }
}
