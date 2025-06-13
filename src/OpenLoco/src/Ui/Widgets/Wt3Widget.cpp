#include "Wt3Widget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    static void sub_4CADE8(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet);

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
                drawingCtx.drawImageSolid(Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 2);
                drawingCtx.drawImageSolid({}, pureImage, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                drawingCtx.drawImageSolid(Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid({}, pureImage, c);
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

        drawingCtx.drawImage({}, imageId);
    }

    void Wt3Widget::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto size = widget.size();

        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        if (widget.content == Widget::kContentUnk)
        {
            flags |= Gfx::RectInsetFlags::fillNone;
            drawingCtx.fillRectInset({}, size, widgetState.colour, flags);
            return;
        }

        if (window->hasFlags(WindowFlags::flag_6))
        {
            drawingCtx.fillRect({}, size, enumValue(ExtColour::unk34), Gfx::RectFlags::transparent);
        }

        drawingCtx.fillRectInset({ 0, 0 }, size, widgetState.colour, flags);

        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        sub_4CADE8(drawingCtx, widget, widgetState);
    }

}
